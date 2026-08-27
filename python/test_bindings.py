"""Smoke-test the compiled pybind11 module from its CMake output directory."""

from __future__ import annotations

import math
import sys


sys.path.insert(0, sys.argv[1])

import _fastmc  # noqa: E402


option = _fastmc.Option(100.0, 100.0, 1.0, _fastmc.OptionType.Call)
market = _fastmc.Market(0.05, 0.20, 0.0)
simulation = _fastmc.Simulation()
simulation.paths = 100_000
simulation.time_steps = 64
simulation.seed = 42
simulation.threads = 2
simulation.antithetic = True

result = _fastmc.price_european(option, market, simulation)
analytic = _fastmc.black_scholes_price(option, market)
greeks = _fastmc.estimate_greeks(option, market, simulation)

assert result.paths == 100_000
assert result.effective_samples == 50_000
assert abs(result.price - analytic) < 5.0 * result.standard_error
assert result.confidence_low < result.price < result.confidence_high
assert math.isfinite(greeks.delta) and math.isfinite(greeks.gamma) and math.isfinite(greeks.vega)

try:
    _fastmc.black_scholes_price(
        _fastmc.Option(100.0, 100.0, 0.0, _fastmc.OptionType.Call), market
    )
except ValueError:
    pass
else:
    raise AssertionError("analytical binding accepted an invalid option")
