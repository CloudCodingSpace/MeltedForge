cmake_minimum_required(VERSION 3.30)

include(cmake/global.cmake)

add_subdirectory(MeltedForge)

option(MF_BUILD_TEST "Build MFTest executable" ON)

if(MF_BUILD_TEST)
    add_subdirectory(MFTest)
endif()
