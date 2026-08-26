# FastMC

A small high-performance Monte Carlo engine for derivatives pricing, written in modern C++ with an optional Python binding.

FastMC is designed to show the engineering underneath numerical finance libraries: deterministic random streams, antithetic sampling, parallel path evaluation, uncertainty estimates, and common-random-number Greeks. It is intentionally focused enough that every numerical choice is inspectable.

## Features

- European and arithmetic-average Asian call/put pricing under Black–Scholes dynamics.
- Antithetic variates and configurable multithreading.
- Price standard errors and 95% confidence intervals.
- Delta, gamma, and vega through common-random-number finite differences.
- Dependency-free C++20 core and optional `pybind11` module.
- Analytical Black–Scholes benchmark for validation.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/fastmc-cli --paths 1000000 --threads 4
```

Example:

```text
FastMC European call
price        10.4381
std. error    0.0147
95% CI       [10.4093, 10.4669]
analytic      10.4506
```

## C++ API

```cpp
#include <fastmc/monte_carlo.hpp>

fastmc::Option option{100.0, 100.0, 1.0, fastmc::OptionType::Call};
fastmc::Market market{0.05, 0.20, 0.00};
fastmc::Simulation simulation{1'000'000, 252, 42, 4, true};

auto result = fastmc::price_european(option, market, simulation);
auto greeks = fastmc::estimate_greeks(option, market, simulation);
```

## Performance model

Work is partitioned across independent seeded streams. Each worker accumulates payoff sums and squared sums locally, avoiding synchronization in the simulation loop. Results are reduced once per worker. The core owns no global state, so pricing calls can safely run concurrently.

## Scope

This is an educational pricing engine, not a trading or risk-management system. The current release assumes constant rates and volatility, uses pseudo-random sampling, and does not claim production calibration or model-risk controls.

