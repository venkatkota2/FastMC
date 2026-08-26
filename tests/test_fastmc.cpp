#include <fastmc/monte_carlo.hpp>

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
    const fastmc::Option call{100.0, 100.0, 1.0, fastmc::OptionType::Call};
    const fastmc::Option put{100.0, 100.0, 1.0, fastmc::OptionType::Put};
    const fastmc::Market market{0.05, 0.20, 0.00};
    const fastmc::Simulation simulation{200'000, 64, 1234, 1, true};

    const auto call_result = fastmc::price_european(call, market, simulation);
    const auto put_result = fastmc::price_european(put, market, simulation);
    const double analytic = fastmc::black_scholes_price(call, market);

    assert(call_result.paths == simulation.paths);
    assert(std::abs(call_result.price - analytic) < 4.0 * call_result.standard_error);
    assert(call_result.confidence_low < call_result.price);
    assert(call_result.price < call_result.confidence_high);

    const double parity = call_result.price - put_result.price;
    const double expected_parity = call.spot - call.strike * std::exp(-market.rate * call.maturity);
    assert(std::abs(parity - expected_parity) < 0.15);

    const auto greeks = fastmc::estimate_greeks(call, market, simulation);
    assert(greeks.delta > 0.0 && greeks.delta < 1.0);
    assert(greeks.gamma > 0.0);
    assert(greeks.vega > 0.0);

    const auto asian = fastmc::price_asian_arithmetic(call, market, simulation);
    assert(asian.price > 0.0);
    assert(asian.price < call_result.price);

    bool rejected = false;
    try {
        const fastmc::Option invalid{0.0, 100.0, 1.0, fastmc::OptionType::Call};
        (void)fastmc::price_european(invalid, market, simulation);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    std::cout << "FastMC tests passed\n";
    return 0;
}

