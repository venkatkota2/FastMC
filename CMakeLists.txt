cmake_minimum_required(VERSION 3.20)
project(FastMC VERSION 0.1.0 LANGUAGES CXX)

option(FASTMC_BUILD_TESTS "Build FastMC tests" ON)
option(FASTMC_BUILD_PYTHON "Build the optional pybind11 module" OFF)
option(FASTMC_BUILD_BENCHMARKS "Build the reproducible benchmark program" OFF)
option(FASTMC_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(fastmc src/monte_carlo.cpp)
set_target_properties(fastmc PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(fastmc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(fastmc PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(fastmc PRIVATE /W4 /permissive-)
  if(FASTMC_WARNINGS_AS_ERRORS)
    target_compile_options(fastmc PRIVATE /WX)
  endif()
else()
  target_compile_options(fastmc PRIVATE -Wall -Wextra -Wpedantic)
  if(FASTMC_WARNINGS_AS_ERRORS)
    target_compile_options(fastmc PRIVATE -Werror)
  endif()
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
  set(PYBIND11_FINDPYTHON ON)
  find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
  find_package(pybind11 CONFIG REQUIRED)
  pybind11_add_module(_fastmc python/bindings.cpp)
  target_link_libraries(_fastmc PRIVATE fastmc)
  if(FASTMC_BUILD_TESTS)
    add_test(
      NAME fastmc-python-bindings
      COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python/test_bindings.py
              $<TARGET_FILE_DIR:_fastmc>
    )
  endif()
endif()

if(FASTMC_BUILD_BENCHMARKS)
  add_executable(fastmc-benchmark benchmarks/benchmark.cpp)
  target_link_libraries(fastmc-benchmark PRIVATE fastmc)
endif()
