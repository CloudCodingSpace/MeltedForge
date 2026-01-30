cmake_minimum_required(VERSION 3.30)

function(EnableFlags target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:/Zi>
            $<$<CONFIG:Release>:/O2>
        )
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:Release>:-O3>
        )
    endif()

    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
endfunction()

set(CMAKE_C_STANDARD 23)

add_subdirectory(MeltedForge)

#TestBench
project(MFTest)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/MFTest/src/*.c")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/MFTest/shaders" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/MFTest/meshes" DESTINATION "${CMAKE_BINARY_DIR}")

add_executable(MFTest ${SOURCES})
target_link_libraries(MFTest PRIVATE mf)
EnableFlags(MFTest)
