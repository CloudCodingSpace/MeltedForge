cmake_minimum_required(VERSION 3.5.0)
cmake_policy(SET CMP0069 NEW)
project(nuklear)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

find_package(Vulkan REQUIRED)

add_library(nuklear)
target_sources(nuklear PRIVATE ${SRCS})
target_include_directories(nuklear PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(nuklear PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../glfw/include")
target_link_libraries(nuklear PRIVATE Vulkan::Vulkan)