cmake_minimum_required(VERSION 3.18)
project(PokerEquity LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to an optimized build (benchmarks are meaningless at -O0).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---- Optional CUDA backend ----
# Built only when a CUDA compiler is found (e.g. on the Lambda GPU box).
# Without it, get_equity_cuda() throws and everything else still builds.
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES 86)   # NVIDIA A10; override with -DCMAKE_CUDA_ARCHITECTURES=...
    endif()
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(ENGINE_HAS_CUDA ON)
    message(STATUS "CUDA compiler found: building the GPU backend")
else()
    message(STATUS "No CUDA compiler: CPU-only build (get_equity_cuda will throw)")
endif()

enable_testing()

# ---- Headless libraries ----
add_subdirectory(engine)            # equity engine (+ its tests)
# add_subdirectory(solver)          # TODO: enable once solver/ has sources (CFR core + tests)

# ---- Terminal demo programs (examples/) ----
# RPS regret-matching demo — standalone, no library deps.
add_executable(rps_cfr examples/rps_cfr.cpp)

# Terminal equity calculator — links the engine (replaces the old HTTP backend).
add_executable(equity_cli examples/equity_cli.cpp)
target_link_libraries(equity_cli PRIVATE engine)
