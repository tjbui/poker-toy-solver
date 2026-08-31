cmake_minimum_required(VERSION 3.16)
project(PokerEquity LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
