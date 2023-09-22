cmake_minimum_required(VERSION 3.23)

project(bloom_filter_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

add_compile_options(-Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=bounds -Werror=implicit-fallthrough=0)
add_link_options(-fsanitize=bounds)
add_executable(${PROJECT_NAME})

# Taken from https://google.github.io/googletest/quickstart-cmake.html
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)

find_package(OpenSSL REQUIRED)

#target_include_directories(${PROJECT_NAME} PRIVATE .)
target_sources(${PROJECT_NAME} PRIVATE bf_test.cc)
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES} gtest_main)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
