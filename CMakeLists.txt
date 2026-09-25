cmake_minimum_required(VERSION 3.24)
project(UHTCNanoDB LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CUDA
find_package(CUDAToolkit REQUIRED)

# OpenVDB/NanoVDB headers (standalone for now)
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/nanodb
  ${CMAKE_CURRENT_SOURCE_DIR}/picogk/API
  ${CMAKE_CURRENT_SOURCE_DIR}/openvdb
)

# PicoGK API tests
add_executable(picogk_api_test
  tests/picogk_api_test.cpp
)
target_link_libraries(picogk_api_test PRIVATE CUDA::cudart)

add_executable(voxel_basic_test
  tests/voxel_basic_test.cpp
)
target_link_libraries(voxel_basic_test PRIVATE CUDA::cudart)

add_executable(uhtc_voxel_analysis_test
  tests/uhtc_voxel_analysis_test.cpp
)
target_link_libraries(uhtc_voxel_analysis_test PRIVATE CUDA::cudart)

add_executable(pnanovdb_portable_test
  tests/pnanovdb_portable_test.cpp
)
target_link_libraries(pnanovdb_portable_test PRIVATE CUDA::cudart)

add_executable(printer_slicer_test
  tests/printer_slicer_test.cpp
)
target_link_libraries(printer_slicer_test PRIVATE CUDA::cudart)

add_executable(rocket_toolpath_test
  tests/rocket_toolpath_test.cpp
)
target_link_libraries(rocket_toolpath_test PRIVATE CUDA::cudart)

# Optional PicoGK viewer (requires OpenGL/GLFW)
option(BUILD_PICOGK_VIEWER "Build PicoGK GL viewer" OFF)
if(BUILD_PICOGK_VIEWER)
  find_package(OpenGL REQUIRED)
  find_package(glfw3 REQUIRED)
  add_executable(picogk_viewer
    picogk/Source/PicoGKGLViewer.cpp
    picogk/Source/PicoGKGLViewerGui.cpp
    picogk/Source/PicoGKGLViewerManager.cpp
    picogk/Source/PicoGKGLViewerObjects.cpp
    picogk/Source/PicoGKGLViewerShaders.cpp
  )
  target_link_libraries(picogk_viewer PRIVATE OpenGL::GL glfw CUDA::cudart)
endif()

# CUDA voxel test (compiles .cu if available)
enable_language(CUDA)
add_executable(voxel_cuda_test
  tests/voxel_cuda_test.cu
)
target_link_libraries(voxel_cuda_test PRIVATE CUDA::cudart)

add_executable(voxel_sdf_cuda_test
  tests/voxel_sdf_cuda_test.cu
)
target_link_libraries(voxel_sdf_cuda_test PRIVATE CUDA::cudart)

add_executable(cuda_thermal_test
  tests/cuda_thermal_test.cu
)
target_link_libraries(cuda_thermal_test PRIVATE CUDA::cudart)

add_executable(cuda_rocket_test
  tests/cuda_rocket_test.cu
)
target_link_libraries(cuda_rocket_test PRIVATE CUDA::cudart)

# Tests
enable_testing()
add_test(NAME picogk_api_test COMMAND picogk_api_test)
add_test(NAME voxel_basic_test COMMAND voxel_basic_test)
add_test(NAME uhtc_voxel_analysis_test COMMAND uhtc_voxel_analysis_test)
add_test(NAME pnanovdb_portable_test COMMAND pnanovdb_portable_test)
add_test(NAME printer_slicer_test COMMAND printer_slicer_test)
add_test(NAME rocket_toolpath_test COMMAND rocket_toolpath_test)
add_test(NAME voxel_cuda_test COMMAND voxel_cuda_test)
add_test(NAME voxel_sdf_cuda_test COMMAND voxel_sdf_cuda_test)
add_test(NAME cuda_thermal_test COMMAND cuda_thermal_test)
add_test(NAME cuda_rocket_test COMMAND cuda_rocket_test)

option(BUILD_ROCKET_ADVANCED "Build the advanced geometry and design-case module" OFF)
if(BUILD_ROCKET_ADVANCED)
  add_subdirectory(rocket_advanced)
endif()
