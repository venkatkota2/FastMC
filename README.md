# FastMC

A performance-oriented Monte Carlo engine for derivatives pricing, written in modern C++ with an optional Python binding.

FastMC is designed to show the engineering underneath numerical finance libraries: deterministic random streams, antithetic sampling, parallel path evaluation, uncertainty estimates, and common-random-number Greeks. It is intentionally focused enough that every numerical choice is inspectable.

## Features

- European and arithmetic-average Asian call/put pricing under Black–Scholes dynamics.
- Antithetic variates and configurable multithreading.
- Price standard errors and 95% confidence intervals computed from independent antithetic pair averages.
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
price        10.4634
std. error    0.0104
95% CI       [10.4430, 10.4838]
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

`result.paths` reports raw simulated paths. With antithetic sampling,
`result.effective_samples` reports the independent pair averages used to
estimate variance and standard error. Antithetic mode requires an even path
count of at least four.

## Python binding

```bash
python -m pip install pybind11
cmake -S . -B build-python -DFASTMC_BUILD_PYTHON=ON \
  -Dpybind11_DIR="$(python -m pybind11 --cmakedir)"
cmake --build build-python --parallel
ctest --test-dir build-python --output-on-failure
```

The binding exposes options, markets, simulation settings, prices, uncertainty
metadata, `Greeks`, and `estimate_greeks`.

## Performance model

Work is partitioned across independent seeded streams. Each worker accumulates independent path or antithetic-pair estimates locally, avoiding synchronization in the simulation loop. Results are reduced once per worker. The core owns no global state, so pricing calls can safely run concurrently.

## Reproducible benchmarking

Performance claims should be tied to a compiler and machine. Build and run the
included benchmark to measure 100,000 and 1,000,000 raw paths with one, two,
and four threads:

```bash
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release \
  -DFASTMC_BUILD_BENCHMARKS=ON
cmake --build build-bench --parallel
./build-bench/fastmc-benchmark
```

The program prints runtime, raw paths per second, compiler version, and
hardware concurrency so results can be reproduced rather than hand-entered.
See [benchmarks/results-linux-gcc13.md](benchmarks/results-linux-gcc13.md) for
one dated, hardware-labelled run. The benchmark is illustrative, not a claim
that other machines or thread counts will reproduce the same throughput.

## Repository layout

```text
include/fastmc/          public C++ API
src/                     simulation core and CLI
python/                  pybind11 bindings and integration test
benchmarks/              reproducible throughput benchmark
tests/                   deterministic numerical regression tests
```

## Scope

This is an educational pricing engine, not a trading or risk-management system. The current release assumes constant rates and volatility, uses pseudo-random sampling, and does not claim production calibration or model-risk controls.
