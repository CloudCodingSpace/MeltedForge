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

function(CompileShader shader)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
    get_filename_component(FILE_NAME ${shader} NAME)
    set(SPIRV_NAME "${CMAKE_BINARY_DIR}/shaders/${FILE_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV_NAME}
        COMMAND ${GLSLC_EXECUTABLE} -I${CMAKE_CURRENT_SOURCE_DIR}/../MeltedForge/mfassets/shaders ${shader} -o ${SPIRV_NAME}
        DEPENDS ${shader}
        COMMENT "Compiling ${shader} to ${SPIRV_NAME}"
        VERBATIM
    )

    set(SPIRV_SHADERS ${SPIRV_SHADERS} ${SPIRV_NAME} PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE SHADERS_VERT CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert")
file(GLOB_RECURSE SHADERS_FRAG CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag")

foreach(SHADER ${SHADERS_VERT})
    CompileShader(${SHADER})
endforeach()

foreach(SHADER ${SHADERS_FRAG})
    CompileShader(${SHADER})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_SHADERS})

# The project
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/meshes" DESTINATION "${CMAKE_BINARY_DIR}")

add_executable(MFTest ${SOURCES})
target_link_libraries(MFTest PRIVATE mf)
EnableFlags(MFTest)

set_target_properties(MFTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
