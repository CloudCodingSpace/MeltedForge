cmake_minimum_required(VERSION 3.30)

include(cmake/global.cmake)

option(MF_BUILD_TEST "Build MFTest executable" ON)

add_subdirectory(MeltedForge)

if(MF_BUILD_TEST)
    add_subdirectory(MFTest)
endif()
