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
- Binary serialization & deserialization
- WIP Material system

---

## Goals

- Cross-platform (Only on desktop platforms)
- Realistic graphics
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
    (Preferably GCC & G++ or MSVC, but currently clang is not tested and is not supported)
- CMake (Get from [here](https://cmake.org/download/))
- Make if using GCC & G++

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

---

## Technical Details (For developers and nerds)
 - This engine is mostly using C. But there *is* some usage of other languages like C++ since
    3rd party vendors like Dear ImGui and Assimp use it.
 - Currently supports compilers like GCC, G++ and MSVC.
 - Aims at having support for Clang & Clang++, but currently it is not tested and 
    does not have support.
 - Currently tested in and developed on Windows with MSVC and GCC/G++
 - Linux isn't tested yet.

 ---

## Documentation

> **Note:** The documentation is currently a work in progress. The github repo is [here](https://github.com/CloudCodingSpace/MFDocs).

The deployed url for the docs is [here](https://cloudcodingspace.github.io/MFDocs).
It would be great if it would be pointed out for any grammartical errors or any suggestions spotted. If so, then
it would be much appreciated if a pull request is opened in the documentation's repo.
