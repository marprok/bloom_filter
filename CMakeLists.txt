cmake_minimum_required(VERSION 4.2.3)

project(bloom_filter_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

add_compile_options(-Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=bounds)
add_link_options(-fsanitize=bounds)
add_executable(${PROJECT_NAME})

# Taken from https://google.github.io/googletest/quickstart-cmake.html
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

target_sources(${PROJECT_NAME} PRIVATE bf_test.cc)
target_link_libraries(${PROJECT_NAME} gtest_main)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
