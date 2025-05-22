# MeltedForge

**MeltedForge** is a game engine written in **C** using **Vulkan**, with a focus on minimal dependencies, performance, and clean design.

<p align="center">
  <img src="MeltedForge/mfassets/logo.png" alt="MeltedForge Logo" height="128" width="128"/>
</p>

---

## 🚀 Goals

- 🔁 Cross-platform
- 🎮 Support for both **3D and 2D** games
- 🔥 Realistic graphics (PBR coming soon 👀)
- 🧠 Entity Component System (ECS)
- 🎵 Sound system support
- 🕹️ Animation system
- 🧩 Flexible and modular
- 🧰 Built to showcase what **C** can really do in game dev
- 🐢 Low-end spec friendly (hopefully)
- 🙌 Beginner-friendly setup and usage

---

## 📦 Dependencies

- [GLFW](https://www.glfw.org/)
- [slog](https://github.com/cloudcircuit/slog) (structured logging)
- Vulkan SDK (Get from [LunarG](https://vulkan.lunarg.com/))
- [stb](https://github.com/nothings/stb) single-header libs
- A GPU driver with **modern Vulkan support**
- A modern **C & C++** compiler

---

## 🛠️ Build Instructions

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