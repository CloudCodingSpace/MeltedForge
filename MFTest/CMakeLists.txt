cmake_minimum_required(VERSION 3.30)
project(MFTest)

# Shaders
find_program(GLSLC_EXECUTABLE glslc HINTS
    "$ENV{VULKAN_SDK}/Bin"
    "$ENV{VULKAN_SDK}/Bin32"
)

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "GLSLC not found! Make sure to download the vulkan sdk and set the VULKAN_SDK environment variable!")
endif()

file(GLOB_RECURSE SHADERS_VERT CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mftshaders/*.vert")
file(GLOB_RECURSE SHADERS_FRAG CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mftshaders/*.frag")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mftmeshes" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mftshaders" DESTINATION "${CMAKE_BINARY_DIR}")

set(SPIRV_SHADERS "")
foreach(SHADER ${SHADERS_VERT})
    CompileShader(${SHADER} -I${CMAKE_CURRENT_SOURCE_DIR}/../MeltedForge/mfassets/shaders ${CMAKE_BINARY_DIR}/mftshaders)
endforeach()

foreach(SHADER ${SHADERS_FRAG})
    CompileShader(${SHADER} -I${CMAKE_CURRENT_SOURCE_DIR}/../MeltedForge/mfassets/shaders ${CMAKE_BINARY_DIR}/mftshaders)
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_SHADERS})

add_executable(MFTest ${SOURCES})
target_link_libraries(MFTest PRIVATE mf)
EnableFlags(MFTest)

set_target_properties(MFTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
