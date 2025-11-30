cmake_minimum_required(VERSION 3.14)
project(CYK)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CYK
        ans.cpp
        CYK.h
        Grammar.h
        Grammar.cpp
)
set_target_properties(CYK PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_executable(cyk_tests
        tests/cyk_tests.cpp
        CYK.h
        Grammar.h
        Grammar.cpp
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cyk_tests PRIVATE --coverage -O0 -g)
    target_link_options(cyk_tests PRIVATE --coverage)
endif()

add_subdirectory(external/googletest)

enable_testing()

target_include_directories(cyk_tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cyk_tests
        PRIVATE
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(cyk_tests)
