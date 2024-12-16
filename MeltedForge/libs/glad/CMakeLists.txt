cmake_minimum_required(VERSION 3.5.0)
project(glad)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(glad)
target_sources(glad PRIVATE ${SRCS})
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")