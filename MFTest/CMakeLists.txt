cmake_minimum_required(VERSION 3.30)
project(MFTest)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/meshes" DESTINATION "${CMAKE_BINARY_DIR}")

add_executable(MFTest ${SOURCES})
target_link_libraries(MFTest PRIVATE mf)
EnableFlags(MFTest)

set_target_properties(MFTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
