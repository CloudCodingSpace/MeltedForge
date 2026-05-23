# Installation

The engine supports mainly two ways for client devs to develop games and applications using MeltedForge.
Installation guide for each of them is given below.

Some tools to be installed before running the engine:

 - CMake (> v3.30, preferable to use the command line tool)
 - VulkanSDK (Make sure that the sdk is installed properly and the `VULKAN_SDK` environmental variable is defined)
 - Vulkan Runtime tools (Corresponding to the VulkanSDK version)
 - GLSLC compiler (Usually included with the VulkanSDK)
 - A modern C & C++ compiler. Like GCC/G++, MSVC, etc, which support C32 and C++20 (Clang not tested)
 - A GPU which supports Vulkan 1.2 and above

!!! Info
    Make sure that the VulkanSDK and the glslc tools can be found through CMake via `find_package` and such.

---

## Installing the editor

!!! Info
    (The editor docs isn't available yet since the editor is yet to do)

---

## Installing the engine's core

!!! Note
    It is preferable that the client developer must have some basic knowledge of CMake before using the engine's core directly. It is also assumed that git is used as the version control.

Clone the [repo](https://github.com/CloudCodingSpace/MeltedForge). Commands:
```bash title="shell"
git clone --recursive https://github.com/CloudCodingSpace/MeltedForge.git
```

You can add it as a submodule to your github repo too. Commands:
```bash title="shell"
git submodule add https://github.com/CloudCodingSpace/MeltedForge.git
git submodule update --init --recursive
```

Then in your client `CMakeLists.txt`, do `add_subdirectory(path/to/MeltedForge)`. Then lastly, link the core to your 
cmake project.

Sample:
```cmake title="CMakeLists.txt" 
...

add_subdirectory(path/to/MeltedForge)

...

add_executable(your_project ${YOUR_SOURCES})
target_link_libraries(your_project PRIVATE mf)
```

Also, it is preferred that you disable the build of the `MFTest` target in CMake. If so, then add this line before adding MeltedForge as a subdirectory:

```cmake title="CMakeLists.txt"
...
set(MF_BUILD_TEST OFF CACHE BOOL "" FORCE)

add_subdirectory(path/to/MeltedForge)

...
```