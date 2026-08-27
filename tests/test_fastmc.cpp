#include <fastmc/monte_carlo.hpp>

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
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
    assert(call_result.effective_samples == simulation.paths / 2);
    assert(std::abs(call_result.price - analytic) < 4.0 * call_result.standard_error);
    assert(call_result.confidence_low < call_result.price);
    assert(call_result.price < call_result.confidence_high);

    const double parity = call_result.price - put_result.price;
    const double expected_parity = call.spot - call.strike * std::exp(-market.rate * call.maturity);
    assert(std::abs(parity - expected_parity) < 0.15);

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
    assert(std::abs(greeks.delta - analytic_delta) < 0.02);
    assert(std::abs(greeks.gamma - analytic_gamma) < 0.005);
    assert(std::abs(greeks.vega - analytic_vega) < 2.0);

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

    rejected = false;
    try {
        auto odd = simulation;
        odd.paths = 101;
        (void)fastmc::price_european(call, market, odd);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        auto invalid_market = market;
        invalid_market.rate = std::numeric_limits<double>::quiet_NaN();
        (void)fastmc::price_european(call, invalid_market, simulation);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    const auto repeated = fastmc::price_european(call, market, simulation);
    assert(repeated.price == call_result.price);
    assert(repeated.standard_error == call_result.standard_error);

    std::cout << "FastMC tests passed\n";
    return 0;
}
