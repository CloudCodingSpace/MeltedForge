# Meltedforge
A game engine created in C using Vulkan with minimal dependencies

<img src="MeltedForge/mfassets/logo.png" alt="MeltedForge Logo" height=128 width=128/>

# Build Instructions
Make sure to clone this repository **recursively**. Like :- 

    git clone --recursive https://github.com/CloudCodingSpace/MeltedForge.git

The make change the directory into the repo's remote folder/directory. Then create a folder/directory
like bin/out/build for the binary output. Then run the following commands :- 

    cmake -S . -B <path-to-build-dir>
    cmake --build <path-to-build-dir> --parallel

# Targets
 - Be cross-platform
 - Have nice realistic graphics
 - Support 3D and 2D games eventually
 - Be flexible
 - Be user friendly
 - Show people what C is really capable of in the game industry
 - Be easily accessible
 - Be low-end spec friendly (Hopefully)
 - Have a nice animation system
 - Have an ECS
 - Have a sound system

# Dependencies
 - GLFW
 - slog
 - VulkanSDK
 - stb single header libs
 - A GPU Driver supporting modern Vulkan
 - A modern C/C++ compiler