add_executable(poker_backend
    src/main.cpp
)

target_include_directories(poker_backend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(poker_backend
    PRIVATE
        engine
)