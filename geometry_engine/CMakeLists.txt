cmake_minimum_required(VERSION 3.24)
project(GeometryEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(geometry_engine
    src/GeometryGenerators.cpp
    src/NozzleProfile.cpp
)
target_include_directories(geometry_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(nozzle_profile_test tests/test_nozzle_profile.cpp)
target_link_libraries(nozzle_profile_test PRIVATE geometry_engine)

add_executable(geometry_generators_test tests/test_geometry_generators.cpp)
target_link_libraries(geometry_generators_test PRIVATE geometry_engine)

enable_testing()
add_test(NAME nozzle_profile_test COMMAND nozzle_profile_test)
add_test(NAME geometry_generators_test COMMAND geometry_generators_test)