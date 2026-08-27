#include <fastmc/monte_carlo.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <thread>
#include <vector>

namespace fastmc {
namespace {

class NormalGenerator {
public:
    explicit NormalGenerator(std::uint64_t seed) : state_(seed) {}

    double next() {
        if (has_spare_) {
            has_spare_ = false;
            return spare_;
        }
        const double u1 = std::max(uniform(), std::numeric_limits<double>::min());
        const double u2 = uniform();
        const double magnitude = std::sqrt(-2.0 * std::log(u1));
        const double angle = 2.0 * std::numbers::pi * u2;
        spare_ = magnitude * std::sin(angle);
        has_spare_ = true;
        return magnitude * std::cos(angle);
    }

private:
    std::uint64_t next_u64() {
        state_ += 0x9e3779b97f4a7c15ULL;
        std::uint64_t value = state_;
        value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31U);
    }

    double uniform() {
        return static_cast<double>(next_u64() >> 11U) * 0x1.0p-53;
    }

    std::uint64_t state_;
    bool has_spare_{false};
    double spare_{0.0};
};

struct Accumulator {
    long double sum{0.0L};
    long double sum_squares{0.0L};
    std::size_t count{0};

    void add(double value) {
        sum += value;
        sum_squares += value * value;
        ++count;
    }
};

void validate_option_market(const Option& option, const Market& market) {
    if (!std::isfinite(option.spot) || !std::isfinite(option.strike)
        || !std::isfinite(option.maturity) || !std::isfinite(market.rate)
        || !std::isfinite(market.volatility) || !std::isfinite(market.dividend_yield)) {
        throw std::invalid_argument("option and market inputs must be finite");
    }
    if (option.spot <= 0.0 || option.strike <= 0.0 || option.maturity <= 0.0) {
        throw std::invalid_argument("spot, strike, and maturity must be positive");
    }
    if (market.volatility < 0.0) {
        throw std::invalid_argument("volatility must be non-negative");
    }
}

void validate(const Option& option, const Market& market, const Simulation& simulation) {
    validate_option_market(option, market);
    if (simulation.time_steps == 0) {
        throw std::invalid_argument("time_steps must be positive");
    }
    if (simulation.antithetic) {
        if (simulation.paths < 4 || simulation.paths % 2 != 0) {
            throw std::invalid_argument(
                "antithetic simulation requires an even path count of at least four"
            );
        }
    } else if (simulation.paths < 2) {
        throw std::invalid_argument("simulation requires at least two paths");
    }
}

double payoff(double underlying, const Option& option) {
    if (option.type == OptionType::Call) {
        return std::max(underlying - option.strike, 0.0);
    }
    return std::max(option.strike - underlying, 0.0);
}

using PathPayoff = double (*)(const Option&, const Market&, std::size_t, NormalGenerator&, double);

double european_path(
    const Option& option,
    const Market& market,
    std::size_t,
    NormalGenerator& generator,
    double sign
) {
    const double z = sign * generator.next();
    const double drift =
        (market.rate - market.dividend_yield - 0.5 * market.volatility * market.volatility)
        * option.maturity;
    const double diffusion = market.volatility * std::sqrt(option.maturity) * z;
    return payoff(option.spot * std::exp(drift + diffusion), option);
}

double asian_path(
    const Option& option,
    const Market& market,
    std::size_t steps,
    NormalGenerator& generator,
    double sign
) {
    const double dt = option.maturity / static_cast<double>(steps);
    const double drift =
        (market.rate - market.dividend_yield - 0.5 * market.volatility * market.volatility) * dt;
    const double diffusion = market.volatility * std::sqrt(dt);
    double spot = option.spot;
    long double sum = 0.0L;
    for (std::size_t step = 0; step < steps; ++step) {
        spot *= std::exp(drift + diffusion * sign * generator.next());
        sum += spot;
    }
    return payoff(static_cast<double>(sum / static_cast<long double>(steps)), option);
}

PriceResult simulate(
    const Option& option,
    const Market& market,
    const Simulation& simulation,
    PathPayoff path_payoff
) {
    validate(option, market, simulation);
    const std::size_t independent_samples =
        simulation.antithetic ? simulation.paths / 2 : simulation.paths;
    unsigned int thread_count = simulation.threads;
    if (thread_count == 0) {
        thread_count = std::max(1U, std::thread::hardware_concurrency());
    }
    thread_count = static_cast<unsigned int>(
        std::min<std::size_t>(thread_count, independent_samples)
    );

    std::vector<Accumulator> partials(thread_count);
    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    const std::size_t base = independent_samples / thread_count;
    const std::size_t remainder = independent_samples % thread_count;

    for (unsigned int worker = 0; worker < thread_count; ++worker) {
        const std::size_t count = base + (worker < remainder ? 1U : 0U);
        workers.emplace_back([&, worker, count] {
            NormalGenerator generator(
                simulation.seed + 0x9e3779b97f4a7c15ULL * static_cast<std::uint64_t>(worker + 1)
            );
            for (std::size_t sample = 0; sample < count; ++sample) {
                if (simulation.antithetic) {
                    const auto snapshot = generator;
                    const double positive =
                        path_payoff(option, market, simulation.time_steps, generator, 1.0);
                    auto antithetic_generator = snapshot;
                    const double negative = path_payoff(
                        option,
                        market,
                        simulation.time_steps,
                        antithetic_generator,
                        -1.0
                    );
                    partials[worker].add(0.5 * (positive + negative));
                } else {
                    partials[worker].add(
                        path_payoff(option, market, simulation.time_steps, generator, 1.0)
                    );
                }
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    Accumulator total;
    for (const auto& part : partials) {
        total.sum += part.sum;
        total.sum_squares += part.sum_squares;
        total.count += part.count;
    }
    const long double mean = total.sum / static_cast<long double>(total.count);
    const long double variance = std::max(
        0.0L,
        (total.sum_squares - total.sum * total.sum / static_cast<long double>(total.count))
            / static_cast<long double>(total.count - 1)
    );
    const double discount = std::exp(-market.rate * option.maturity);
    const double price = discount * static_cast<double>(mean);
    const double standard_error =
        discount * std::sqrt(static_cast<double>(variance) / static_cast<double>(total.count));
    if (!std::isfinite(price) || !std::isfinite(standard_error)) {
        throw std::overflow_error("simulation produced a non-finite result");
    }
    constexpr double z95 = 1.959963984540054;
    return {
        price,
        standard_error,
        price - z95 * standard_error,
        price + z95 * standard_error,
        simulation.paths,
        total.count,
    };
}

double normal_cdf(double value) {
    return 0.5 * std::erfc(-value / std::sqrt(2.0));
}

}  // namespace

PriceResult price_european(
    const Option& option,
    const Market& market,
    const Simulation& simulation
) {
    return simulate(option, market, simulation, european_path);
}

PriceResult price_asian_arithmetic(
    const Option& option,
    const Market& market,
    const Simulation& simulation
) {
    return simulate(option, market, simulation, asian_path);
}

Greeks estimate_greeks(
    const Option& option,
    const Market& market,
    const Simulation& simulation
) {
    validate(option, market, simulation);
    // Every bumped valuation receives identical Simulation settings. The
    // deterministic streams therefore implement common random numbers.
    const double spot_bump = std::min(option.spot * 0.5, std::max(1e-6, option.spot * 0.01));
    const double volatility_bump = 0.001;
    Option down = option;
    Option up = option;
    down.spot -= spot_bump;
    up.spot += spot_bump;
    const double down_price = price_european(down, market, simulation).price;
    const double base_price = price_european(option, market, simulation).price;
    const double up_price = price_european(up, market, simulation).price;

    Market volatility_up = market;
    volatility_up.volatility += volatility_bump;
    const double bumped_volatility_price = price_european(option, volatility_up, simulation).price;

    return {
        (up_price - down_price) / (2.0 * spot_bump),
        (up_price - 2.0 * base_price + down_price) / (spot_bump * spot_bump),
        (bumped_volatility_price - base_price) / volatility_bump,
    };
}

double black_scholes_price(const Option& option, const Market& market) {
    validate_option_market(option, market);
    if (market.volatility == 0.0) {
        const double forward_spot =
            option.spot * std::exp((market.rate - market.dividend_yield) * option.maturity);
        return std::exp(-market.rate * option.maturity) * payoff(forward_spot, option);
    }
    const double root_time = std::sqrt(option.maturity);
    const double d1 =
        (std::log(option.spot / option.strike)
         + (market.rate - market.dividend_yield + 0.5 * market.volatility * market.volatility)
             * option.maturity)
        / (market.volatility * root_time);
    const double d2 = d1 - market.volatility * root_time;
    const double discounted_spot =
        option.spot * std::exp(-market.dividend_yield * option.maturity);
    const double discounted_strike = option.strike * std::exp(-market.rate * option.maturity);
    if (option.type == OptionType::Call) {
        return discounted_spot * normal_cdf(d1) - discounted_strike * normal_cdf(d2);
    }
    return discounted_strike * normal_cdf(-d2) - discounted_spot * normal_cdf(-d1);
}

}  // namespace fastmc
