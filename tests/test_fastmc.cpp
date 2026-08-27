#include <fastmc/monte_carlo.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

}  // namespace

int main() {
    const fastmc::Option call{100.0, 100.0, 1.0, fastmc::OptionType::Call};
    const fastmc::Option put{100.0, 100.0, 1.0, fastmc::OptionType::Put};
    const fastmc::Market market{0.05, 0.20, 0.00};
    const fastmc::Simulation simulation{200'000, 64, 1234, 1, true};

    const auto call_result = fastmc::price_european(call, market, simulation);
    const auto put_result = fastmc::price_european(put, market, simulation);
    const double analytic = fastmc::black_scholes_price(call, market);

    require(call_result.paths == simulation.paths, "raw path count mismatch");
    require(call_result.effective_samples == simulation.paths / 2, "pair count mismatch");
    require(
        std::abs(call_result.price - analytic) < 4.0 * call_result.standard_error,
        "Monte Carlo price does not reconcile to Black-Scholes"
    );
    require(call_result.confidence_low < call_result.price, "invalid lower confidence bound");
    require(call_result.price < call_result.confidence_high, "invalid upper confidence bound");

    const double parity = call_result.price - put_result.price;
    const double expected_parity = call.spot - call.strike * std::exp(-market.rate * call.maturity);
    require(std::abs(parity - expected_parity) < 0.15, "put-call parity regression");

    const auto greeks = fastmc::estimate_greeks(call, market, simulation);
    const double d1 =
        (std::log(call.spot / call.strike)
         + (market.rate + 0.5 * market.volatility * market.volatility) * call.maturity)
        / (market.volatility * std::sqrt(call.maturity));
    const double density = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * std::acos(-1.0));
    const double analytic_delta = 0.5 * std::erfc(-d1 / std::sqrt(2.0));
    const double analytic_gamma =
        density / (call.spot * market.volatility * std::sqrt(call.maturity));
    const double analytic_vega = call.spot * density * std::sqrt(call.maturity);
    require(std::abs(greeks.delta - analytic_delta) < 0.02, "delta regression");
    require(std::abs(greeks.gamma - analytic_gamma) < 0.005, "gamma regression");
    require(std::abs(greeks.vega - analytic_vega) < 2.0, "vega regression");

    const auto asian = fastmc::price_asian_arithmetic(call, market, simulation);
    require(asian.price > 0.0, "Asian price must be positive");
    require(asian.price < call_result.price, "Asian price regression");

    bool rejected = false;
    try {
        const fastmc::Option invalid{0.0, 100.0, 1.0, fastmc::OptionType::Call};
        (void)fastmc::price_european(invalid, market, simulation);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "zero spot was not rejected");

    rejected = false;
    try {
        auto odd = simulation;
        odd.paths = 101;
        (void)fastmc::price_european(call, market, odd);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "odd antithetic path count was not rejected");

    rejected = false;
    try {
        auto invalid_market = market;
        invalid_market.rate = std::numeric_limits<double>::quiet_NaN();
        (void)fastmc::price_european(call, invalid_market, simulation);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "non-finite market input was not rejected");

    rejected = false;
    try {
        const fastmc::Option invalid{100.0, 100.0, 0.0, fastmc::OptionType::Call};
        (void)fastmc::black_scholes_price(invalid, market);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "analytical pricer did not validate its public inputs");

    const fastmc::Option small_spot{0.005, 0.005, 1.0, fastmc::OptionType::Call};
    const auto small_spot_greeks = fastmc::estimate_greeks(small_spot, market, simulation);
    require(std::isfinite(small_spot_greeks.delta), "small-spot delta is not finite");

    const auto repeated = fastmc::price_european(call, market, simulation);
    require(repeated.price == call_result.price, "price is not reproducible");
    require(repeated.standard_error == call_result.standard_error, "error is not reproducible");

    std::cout << "FastMC tests passed\n";
    return 0;
}
