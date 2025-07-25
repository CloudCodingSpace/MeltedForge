cmake_minimum_required(VERSION 3.30)
project(mf C CXX)

cmake_policy(SET CMP0069 NEW)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mfassets" DESTINATION "${CMAKE_BINARY_DIR}")

add_subdirectory(libs/glfw)
add_subdirectory(libs/stb)
add_subdirectory(libs/slog)
add_subdirectory(libs/tinyobj)

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

add_library(mf ${SRCS})
target_include_directories(mf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(mf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/mf")
target_link_libraries(mf PUBLIC stb slog cimgui tinyobj)
target_link_libraries(mf PRIVATE glfw Vulkan::Vulkan)
set_property(TARGET mf PROPERTY C_STANDARD 23)