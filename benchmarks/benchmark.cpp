#include <fastmc/monte_carlo.hpp>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

int main() {
    const fastmc::Option option{100.0, 100.0, 1.0, fastmc::OptionType::Call};
    const fastmc::Market market{0.05, 0.20, 0.00};

    std::cout << "compiler=";
#if defined(_MSC_VER)
    std::cout << "MSVC " << _MSC_VER;
#else
    std::cout << __VERSION__;
#endif
    std::cout << '\n'
              << "hardware_concurrency=" << std::thread::hardware_concurrency() << '\n'
              << "paths,threads,seconds,paths_per_second,price\n";
    for (const std::size_t paths : {100'000U, 1'000'000U}) {
        for (const unsigned int threads : {1U, 2U, 4U}) {
            const fastmc::Simulation simulation{paths, 252, 42, threads, true};
            const auto start = std::chrono::steady_clock::now();
            const auto result = fastmc::price_european(option, market, simulation);
            const auto elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start
            ).count();
            std::cout << paths << ',' << threads << ',' << std::fixed << std::setprecision(6)
                      << elapsed << ',' << static_cast<double>(paths) / elapsed << ','
                      << result.price << '\n';
        }
    }
}
