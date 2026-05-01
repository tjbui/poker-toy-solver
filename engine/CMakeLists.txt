add_library(engine
    src/evaluator.cpp
)

target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_eval_basic
    tests/test_eval_basic.cpp
)

target_link_libraries(test_eval_basic
    PRIVATE engine
)

add_executable(test_eval_flush
    tests/test_eval_flush.cpp
)

target_link_libraries(test_eval_flush
    PRIVATE engine
)