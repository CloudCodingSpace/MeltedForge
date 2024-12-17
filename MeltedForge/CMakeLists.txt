cmake_minimum_required(VERSION 3.14)
project(mf C)

cmake_policy(SET CMP0069 NEW)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_subdirectory(libs/GLFW)
add_subdirectory(libs/stb)
add_subdirectory(libs/slog)
add_subdirectory(libs/glad)

find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(_DEBUG)
else()
  add_compile_definitions(NDEBUG)
endif()

add_library(mf ${SRCS})
target_include_directories(mf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libs")
target_include_directories(mf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(mf PUBLIC glfw glad stb slog Vulkan::Vulkan OpenGL::GL)
