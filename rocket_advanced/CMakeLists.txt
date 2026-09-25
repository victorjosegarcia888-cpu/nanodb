cmake_minimum_required(VERSION 3.24)
project(RocketAdvanced LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CUDAToolkit REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../nanodb
    ${CMAKE_CURRENT_SOURCE_DIR}/../picogk/API
    ${CMAKE_CURRENT_SOURCE_DIR}/../openvdb
)

add_executable(advanced_rocket_test
    src/AdvancedRocketGeometries.cu
    tests/test_advanced_rocket.cu
)
target_link_libraries(advanced_rocket_test PRIVATE CUDA::cudart)

add_executable(design_case_test
    tests/test_design_case.cpp
)
target_include_directories(design_case_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()
add_test(NAME advanced_rocket_test COMMAND advanced_rocket_test)
add_test(NAME design_case_test COMMAND design_case_test)
