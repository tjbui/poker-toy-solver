add_library(engine SHARED
    src/evaluator.cpp
    src/simulation.cpp
    src/engine_iterative.cpp
)

target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(engine_test
    src/test_main.cpp
)

target_link_libraries(engine_test
    PRIVATE
        engine
)