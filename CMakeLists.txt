if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  # Use `cmake . -Bbuild` to get build files.
  # Use `cmake --build build --config Release` to build dependencies.
  message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the SDL source code and call cmake from there")
endif()

cmake_minimum_required(VERSION 3.5)
project(Rays)

add_subdirectory(hamjet)

file(GLOB inFiles "${Rays_SOURCE_DIR}/src/*.cpp")

add_executable(rays ${inFiles})
target_link_libraries(rays libhamjet)
