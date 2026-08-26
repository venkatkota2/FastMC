#include <fastmc/monte_carlo.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    fastmc::Simulation simulation;
    for (int index = 1; index + 1 < argc; index += 2) {
        const std::string_view flag(argv[index]);
        if (flag == "--paths") {
            simulation.paths = std::strtoull(argv[index + 1], nullptr, 10);
        } else if (flag == "--threads") {
            simulation.threads = static_cast<unsigned int>(std::strtoul(argv[index + 1], nullptr, 10));
        } else if (flag == "--seed") {
            simulation.seed = std::strtoull(argv[index + 1], nullptr, 10);
        }
    }

    const fastmc::Option option{100.0, 100.0, 1.0, fastmc::OptionType::Call};
    const fastmc::Market market{0.05, 0.20, 0.00};
    const auto result = fastmc::price_european(option, market, simulation);

    std::cout << std::fixed << std::setprecision(4)
              << "FastMC European call\n"
              << "price        " << result.price << '\n'
              << "std. error   " << result.standard_error << '\n'
              << "95% CI       [" << result.confidence_low << ", " << result.confidence_high
              << "]\n"
              << "analytic     " << fastmc::black_scholes_price(option, market) << '\n';
    return 0;
}

