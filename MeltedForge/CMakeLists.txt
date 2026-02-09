cmake_minimum_required(VERSION 3.30)
project(mf C CXX)

cmake_policy(SET CMP0069 NEW)

set(SLOG_BUILD_TEST OFF CACHE BOOL "" FORCE)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mfassets" DESTINATION "${CMAKE_BINARY_DIR}")

add_subdirectory(libs/glfw)
add_subdirectory(libs/stb)
add_subdirectory(libs/slog)
add_subdirectory(libs/assimp)

find_package(Vulkan REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(_DEBUG)
else()
  add_compile_definitions(NDEBUG)
endif()

#cimgui
add_library(cimgui STATIC)
target_sources(cimgui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/cimgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/cimgui_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/imgui/backends/imgui_impl_glfw.cpp
)

target_include_directories(cimgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui)
target_include_directories(cimgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/imgui)
target_include_directories(cimgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs/cimgui/imgui/backends)
target_compile_definitions(cimgui PUBLIC "CIMGUI_USE_GLFW" "CIMGUI_USE_VULKAN")
target_compile_definitions(cimgui PUBLIC "IMGUI_USER_CONFIG=\"../cimconfig.h\"")
target_compile_definitions(cimgui PUBLIC "IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1")
target_compile_definitions(cimgui PUBLIC "CIMGUI_NO_EXPORT")
target_compile_definitions(cimgui PUBLIC "IMGUI_IMPL_API=extern \"C\"")
target_link_libraries(cimgui PRIVATE glfw Vulkan::Vulkan)

function(EnableFlags target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:/Zi>
            $<$<CONFIG:Release>:/O2>
        )
    else()
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:Release>:-O3>
        )
    endif()

    if(WIN32)
        set(MF_PLATFORM "MF_PLATFORM_WINDOWS")
    elseif(APPLE)
        set(MF_PLATFORM "MF_PLATFORM_MAC")
    elseif(UNIX AND NOT APPLE)
        set(MF_PLATFORM "MF_PLATFORM_LINUX")
    endif()

    target_compile_definitions(${target} PUBLIC
        $<$<CONFIG:Debug>:_DEBUG;MF_DEBUG>
        $<$<CONFIG:Release>:NDEBUG;MF_NDEBUG>
        ${MF_PLATFORM}
    )
endfunction()

add_library(mf ${SRCS})
target_include_directories(mf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(mf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/mf")
target_link_libraries(mf PUBLIC stb slog cimgui assimp)
target_link_libraries(mf PRIVATE glfw Vulkan::Vulkan)
set_property(TARGET mf PROPERTY C_STANDARD 23)
EnableFlags(mf)
