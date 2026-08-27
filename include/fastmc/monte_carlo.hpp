#pragma once

#include <cstddef>
#include <cstdint>

namespace fastmc {

enum class OptionType { Call, Put };

struct Option {
    double spot;
    double strike;
    double maturity;
    OptionType type;
};

struct Market {
    double rate;
    double volatility;
    double dividend_yield;
};

struct Simulation {
    std::size_t paths{1'000'000};
    std::size_t time_steps{252};
    std::uint64_t seed{42};
    unsigned int threads{0};
    bool antithetic{true};
};

struct PriceResult {
    double price;
    double standard_error;
    double confidence_low;
    double confidence_high;
    std::size_t paths;
    std::size_t effective_samples;
};

struct Greeks {
    double delta;
    double gamma;
    double vega;
};

PriceResult price_european(const Option& option, const Market& market, const Simulation& simulation);
PriceResult price_asian_arithmetic(
    const Option& option,
    const Market& market,
    const Simulation& simulation
);
Greeks estimate_greeks(const Option& option, const Market& market, const Simulation& simulation);
double black_scholes_price(const Option& option, const Market& market);

}  // namespace fastmc
