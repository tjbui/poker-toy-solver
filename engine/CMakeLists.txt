add_library(engine
    src/evaluator.cpp
    src/simulation.cpp
    src/engine_cpu_iterative.cpp
    src/engine_cpu_multithr.cpp
)

target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# CUDA backend when nvcc is available, otherwise a stub that throws.
if(ENGINE_HAS_CUDA)
    find_package(CUDAToolkit REQUIRED)
    target_sources(engine PRIVATE src/engine_cuda.cu)
    target_link_libraries(engine PUBLIC CUDA::cudart)
    target_compile_definitions(engine PUBLIC ENGINE_HAS_CUDA)
else()
    target_sources(engine PRIVATE src/engine_cuda_stub.cpp)
endif()

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
add_engine_test(test_equity_sim)

if(ENGINE_HAS_CUDA)
    add_engine_test(test_equity_cuda)   # GPU must match CPU exactly
endif()
