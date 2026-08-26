cmake_minimum_required(VERSION 3.20)
project(FastMC VERSION 0.1.0 LANGUAGES CXX)

option(FASTMC_BUILD_TESTS "Build FastMC tests" ON)
option(FASTMC_BUILD_PYTHON "Build the optional pybind11 module" OFF)

add_library(fastmc src/monte_carlo.cpp)
target_include_directories(fastmc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(fastmc PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(fastmc PRIVATE /W4 /permissive-)
else()
  target_compile_options(fastmc PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(fastmc-cli src/main.cpp)
target_link_libraries(fastmc-cli PRIVATE fastmc)

if(FASTMC_BUILD_TESTS)
  enable_testing()
  add_executable(fastmc-tests tests/test_fastmc.cpp)
  target_link_libraries(fastmc-tests PRIVATE fastmc)
  add_test(NAME fastmc-tests COMMAND fastmc-tests)
endif()

if(FASTMC_BUILD_PYTHON)
  find_package(pybind11 CONFIG REQUIRED)
  pybind11_add_module(_fastmc python/bindings.cpp)
  target_link_libraries(_fastmc PRIVATE fastmc)
endif()

