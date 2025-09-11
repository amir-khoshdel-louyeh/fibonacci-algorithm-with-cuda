cmake_minimum_required(VERSION 3.24)

project(fibonacci_algorithm_with_cuda LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

add_executable(fibonacci_cuda fibonacci-algorithm-with-cuda.cu)

set_target_properties(fibonacci_cuda PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

include(CTest)

if(BUILD_TESTING)
    add_test(
        NAME fibonacci_help
        COMMAND fibonacci_cuda --help
    )
    set_tests_properties(fibonacci_help PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage:"
    )

    add_test(
        NAME fibonacci_rejects_invalid_count
        COMMAND fibonacci_cuda --count 0
    )
    set_tests_properties(fibonacci_rejects_invalid_count PROPERTIES
        WILL_FAIL TRUE
        PASS_REGULAR_EXPRESSION "--count must be between"
    )
endif()
