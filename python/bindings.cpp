#include <fastmc/monte_carlo.hpp>
#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_fastmc, module) {
    py::enum_<fastmc::OptionType>(module, "OptionType")
        .value("Call", fastmc::OptionType::Call)
        .value("Put", fastmc::OptionType::Put);

    py::class_<fastmc::Option>(module, "Option")
        .def(py::init<double, double, double, fastmc::OptionType>())
        .def_readwrite("spot", &fastmc::Option::spot)
        .def_readwrite("strike", &fastmc::Option::strike)
        .def_readwrite("maturity", &fastmc::Option::maturity)
        .def_readwrite("type", &fastmc::Option::type);

    py::class_<fastmc::Market>(module, "Market")
        .def(py::init<double, double, double>())
        .def_readwrite("rate", &fastmc::Market::rate)
        .def_readwrite("volatility", &fastmc::Market::volatility)
        .def_readwrite("dividend_yield", &fastmc::Market::dividend_yield);

    py::class_<fastmc::Simulation>(module, "Simulation")
        .def(py::init<>())
        .def_readwrite("paths", &fastmc::Simulation::paths)
        .def_readwrite("time_steps", &fastmc::Simulation::time_steps)
        .def_readwrite("seed", &fastmc::Simulation::seed)
        .def_readwrite("threads", &fastmc::Simulation::threads)
        .def_readwrite("antithetic", &fastmc::Simulation::antithetic);

    py::class_<fastmc::PriceResult>(module, "PriceResult")
        .def_readonly("price", &fastmc::PriceResult::price)
        .def_readonly("standard_error", &fastmc::PriceResult::standard_error)
        .def_readonly("confidence_low", &fastmc::PriceResult::confidence_low)
        .def_readonly("confidence_high", &fastmc::PriceResult::confidence_high)
        .def_readonly("paths", &fastmc::PriceResult::paths)
        .def_readonly("effective_samples", &fastmc::PriceResult::effective_samples);

    py::class_<fastmc::Greeks>(module, "Greeks")
        .def_readonly("delta", &fastmc::Greeks::delta)
        .def_readonly("gamma", &fastmc::Greeks::gamma)
        .def_readonly("vega", &fastmc::Greeks::vega);

    module.def("price_european", &fastmc::price_european);
    module.def("price_asian_arithmetic", &fastmc::price_asian_arithmetic);
    module.def("estimate_greeks", &fastmc::estimate_greeks);
    module.def("black_scholes_price", &fastmc::black_scholes_price);
}
