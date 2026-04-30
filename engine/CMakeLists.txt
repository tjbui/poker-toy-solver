add_library(engine
    src/evaluator.cpp
)

target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_evaluator
    src/test_evaluator.cpp
)

target_link_libraries(test_evaluator
    PRIVATE engine
)