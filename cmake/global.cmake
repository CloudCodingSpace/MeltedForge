set(CMAKE_C_STANDARD 23)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(_DEBUG MF_DEBUG)
else()
  add_compile_definitions(NDEBUG MF_NDEBUG)
endif()
add_compile_definitions(TRACY_ENABLE)

find_program(GLSLC_EXECUTABLE glslc REQUIRED)

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "GLSLC not found! Make sure to download the vulkan sdk and set the VULKAN_SDK environment variable!")
endif()

function(EnableFlags target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:/Zi>
            $<$<CONFIG:Release>:/O2>
        )
        set(MF_PROFILE "MF_ENABLE_PROFILING")
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:Release>:-O3>
        )
        set(MF_PROFILE "")
    endif()

    if(WIN32)
        set(MF_PLATFORM "MF_PLATFORM_WINDOWS")
    elseif(APPLE)
        set(MF_PLATFORM "MF_PLATFORM_MAC")
    elseif(UNIX AND NOT APPLE)
        set(MF_PLATFORM "MF_PLATFORM_LINUX")
    endif()

    target_compile_definitions(${target} PUBLIC
        ${MF_PLATFORM}
        ${MF_PROFILE}
    )
endfunction()

set(SPIRV_SHADERS "")

function(CompileShader shader flags output_dir OUT_LIST)
    get_filename_component(FILE_NAME ${shader} NAME)
    set(SPIRV_NAME "${output_dir}/${FILE_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV_NAME}
        COMMAND ${GLSLC_EXECUTABLE} ${flags} ${shader} -o ${SPIRV_NAME}
        DEPENDS ${shader}
        BYPRODUCTS ${SPIRV_NAME}
        VERBATIM
    )

    set(${OUT_LIST} ${${OUT_LIST}} ${SPIRV_NAME} PARENT_SCOPE)
endfunction()