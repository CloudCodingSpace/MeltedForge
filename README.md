# MeltedForge

**MeltedForge** is a game engine written in **C** using **Vulkan**, with a focus on minimal dependencies, performance, and clean design.

<p align="center">
  <img src="MeltedForge/mfassets/logo/logo.png" alt="MeltedForge Logo" height="128" width="128"/>
</p>

---

## Features
- Model loading along with the material data
- Entity Component System (ECS)
- Engine & editor level UI
- UI customization
- Rendering multiple entities
- Render targets (Ability to set the render output to an image, which can be used to render the scene inside an UI panel like the scene viewport)

## Goals

- Cross-platform
- Support for both **3D and 2D** games
- Realistic graphics (PBR coming soon 👀)
- Sound system support
- Animation system
- Flexible and modular
- Built to showcase what **C** can really do in game dev
- Low-end spec friendly (hopefully)
- Beginner-friendly setup and usage
- Multithreading
- Async model loading

---

## Dependencies

> **Note:** The following are the **important conditions** met by the PC for **building/running** MeltedForge

- Vulkan SDK (Get from [LunarG](https://vulkan.lunarg.com/))
- A GPU driver with **modern Vulkan support** (Vulkan 1.2.000+)
- A modern **C & C++** compiler with the support of **latest language standards** with **the corresponding runtime libraries**
- CMake (Get from [here](https://cmake.org/download/))
- Make if using GCC & G++ or Clang & Clang++

---

## Build Instructions

> **Note:** This repo uses submodules. Make sure to clone it **recursively**.

```bash
git clone --recursive https://github.com/CloudCodingSpace/MeltedForge.git
```

The make change the directory into the repo's remote folder/directory. Then create a folder/directory
like bin/out/build for the binary output. Then run the following commands :- 

```bash
cmake -S . -B <path-to-build-dir>
cmake --build <path-to-build-dir> --parallel
```

> **Note:** Before running the executable of the MFTest, make sure that the shaders are compiled. Helper scripts to compile shaders on Window are in the scripts folder.
