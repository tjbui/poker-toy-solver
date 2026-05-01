add_library(engine
    src/evaluator.cpp
    src/simulation.cpp
    src/engine_cpu_iterative.cpp
)

target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_testing()

function(add_engine_test test_name)
    add_executable(${test_name}
        tests/${test_name}.cpp
    )

    target_link_libraries(${test_name}
        PRIVATE engine
    )

    add_test(
        NAME ${test_name}
        COMMAND ${test_name}
    )
endfunction()

add_engine_test(test_eval_basic)
add_engine_test(test_eval_flush)
add_engine_test(test_eval_fullhouse)
add_engine_test(test_eval_pairs)
add_engine_test(test_sim_iterative)