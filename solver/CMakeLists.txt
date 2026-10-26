# ---- toy CFR solver (header-only) ----
#   include/solver/cfr.h    CfrTrainer<Game>: vanilla CFR, written once for any game
#   include/solver/kuhn.h   Kuhn poker
#   include/solver/rps.h    Rock-Paper-Scissors (smoke test for the core)

add_library(solver INTERFACE)
target_include_directories(solver INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---- tests ----
function(add_solver_test test_name)
    add_executable(${test_name} tests/${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE solver)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_solver_test(test_rps)
add_solver_test(test_kuhn)
