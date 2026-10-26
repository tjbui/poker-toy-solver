# ---- equity engine library ----
#   include/equity/  public API (the only headers callers see)
#   src/common/      shared by every backend: workload prep, HD evaluator + combinatorics
#   src/cpu/         single-thread + multithreaded backends
#   src/gpu/         CUDA backend: gpu_backend.cu + header-only .cuh layers (or a stub
#                    when there is no CUDA compiler)
#   src/equity.cpp   compute_equity(): backend dispatch

find_package(Threads REQUIRED)

add_library(engine
    src/equity.cpp
    src/common/workload.cpp
    src/cpu/enumerate.cpp
    src/cpu/multithread.cpp
)

target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(engine PRIVATE Threads::Threads)

# CUDA backend when nvcc is available, otherwise a stub that throws.
if(ENGINE_HAS_CUDA)
    find_package(CUDAToolkit REQUIRED)
    target_sources(engine PRIVATE src/gpu/gpu_backend.cu)
    target_link_libraries(engine PRIVATE CUDA::cudart)
else()
    target_sources(engine PRIVATE src/gpu/gpu_stub.cpp)
endif()

# ---- tests ----
enable_testing()

function(add_engine_test test_name)
    add_executable(${test_name} tests/${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE engine)
    # The evaluator tests exercise internal headers (common/hand_eval.h).
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_engine_test(test_eval_basic)
add_engine_test(test_eval_flush)
add_engine_test(test_eval_fullhouse)
add_engine_test(test_eval_pairs)
add_engine_test(test_equity_cpu)

if(ENGINE_HAS_CUDA)
    add_engine_test(test_equity_gpu)   # GPU must match CPU exactly
endif()
