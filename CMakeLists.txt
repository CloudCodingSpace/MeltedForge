cmake_minimum_required(VERSION 3.8.0)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    add_compile_options(/arch:AVX2)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

set(CMAKE_C_STANDARD 17)

add_subdirectory(MeltedForge)

#TestBench
project(MFTest)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/MFTest/src/*.c")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/MFTest/shaders" DESTINATION "${CMAKE_BINARY_DIR}")

add_executable(MFTest ${SOURCES})
target_link_libraries(MFTest PRIVATE mf)
