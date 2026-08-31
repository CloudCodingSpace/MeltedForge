# MeltedForge

MeltedForge is game engine written in the C language. It was first started as a simple
hobby engine. But slowly, the aim wasn't anymore to be a "hobby" engine. As a game engine,
we want it to be like a proper 2d/3d game engine which can actually export fun and enjoyable games.
We admit it sounds ambitious and like any other "wannabe" engine. But still we continue to work to make 
goal a reality.

The entire code is open sourced. The github repository is [here](https://github.com/CloudCodingSpace/MeltedForge).
One of the main reason it is open sourced is first of all, we feel like open source projects is a great learning resource
for other devs and secondly, the option for community suggestions is open. Which in our opinion, is a good thing since
it helps pointing out bugs, certain inconsistencies and enrich ideas from various minds all over the world.

The point of this engine isn't to compete with industrial standard, and production ready game engines like 
Unity and Unreal Engine, but to serve the purpose of mini-studios and small indie devs, who prefer flexibility, 
bloat free options and lightweightedness of the engine.

!!! Info
    MeltedForge was officially created on April 8, 2025 and is currently licensed under the ZLib license.

---

## Goals

We want to give our client developer 2 main options:

 - Either create a game using the engine's editor
 - Or, manually write your app natively in C/C++ by directly interacting with the engine's core.

We recommend the second option for devs who have prior experience with low-level systems
and want more customization and flexibility without any useless bloat.

---

## Current position

At the moment of writing this documentation, MeltedForge isn't quite matured yet. It has *many* faults and doesn't 
has basic engine features. So long story short, it is in it's young stage. Though it is also true that it is not a *toy* 
test engine either. It is for now, far from the likes of an industrial standard, production ready game 
engine like Unity, Godot, Unreal Engine, Cry engine, etc. but it is in the path of becoming a engine which *can actually* export 
games and applications.

Features of the engine as of now:

 - Window & Input
   - Basic window creation & handling
   - Fullscreen windows
   - Window icons
   - Basic mouse and keyboard input
 - A Basic ECS
   - Multiple entities support
   - Can add/remove components to/from a scene
   - Can attach/detach components from entities
   - Basic scene management
   - Scene load/save
   - Material system
   - Support for fustrum culling (On the CPU as of now)
 - GLTF 2.0 model loading along with the material data
    - Can load complex models (Tested with Sponza & Bistro Internal)
 - Engine & editor level UI
    - UI customization (Using Dear ImGui's styles)
 - RenderGraph
    - A helper tool which manages attachments, passes, framebuffers for us and helps us to render and invoke several interlinked passes
    - Can get each attachment's pixel data on the CPU
    - Can get ImTextureID for each attachment in it for viewing it in ImGui (Provided ui is enabled in the renderer)
    - Can get a dedicated resource set containing every attachment in order as given when creating the rendergraph
    - Currently MSAA is not supported for rendergraph's attachments
 - A binary serialization/deserialization api
 - Explicit gpu resource management control for resources like:
    - Vertex buffers
    - Index buffers
    - Textures and skyboxes
    - UBOs
    - SSBOs
    - Storage images
    - Image samplers (`sampler2D` and `samplerCube` in glsl)
    - Resource set and layout management (MFResourcSet and MFResourceSetLayout)
 - Skybox support
    - Only 2D equirectangular images as input
    - HDR format supported as well
    - Optional to generate utility IBL textures such as BRDF LUT, Prefiltered Map and Irradiance map
 - Basic PBR support
    - Diffuse IBL support
    - Specular IBL support
 - MSAA support for native window
 - Can get the image pixels from the native window, and images as well.
 - Support for compute pipelines and compute dispatches too
 - Can fetch and tell which optional GPU features are available, like Buffer Device Address, descriptor indexing, etc
 - Support for headless rendering, meaning the engine can render to an image even if a window isn't provided

### MFTest's latest status (A test application included in the main repo to stress test the engine's core)
   ![img](https://raw.githubusercontent.com/CloudCodingSpace/MeltedForge/refs/heads/main/MFTest/screenshots/sponza_pbr.png)

!!! Info
   This above image is currently in development and is being run on an Intel UHD so the delta time is more than it should be ideally 😅

---

## Third party dependencies

The engine uses a few third party dependencies internally. They are:

 - Slog (Custom logger by the same author. Repository [here](https://github.com/CloudCodingSpace/slog))
 - Stb's single header utils (Single header utils by stb. Repository [here](https://github.com/CloudCodingSpace/stb))
 - Glfw (Cross-platform windowing/util library. Repository [here](https://github.com/CloudCodingSpace/GLFW/tree/73f09546ccd681047360e4e8bc5474adad55147f))
 - Tracy (For profiling. Repository [here](https://github.com/wolfpld/tracy/tree/05cceee0df3b8d7c6fa87e9638af311dbabc63cb))
 - CGLTF (Model loader. Repository [here](https://github.com/CloudCodingSpace/cgltf))
 - CImGui (C bindings for Dear ImGui, a immediate mode UI renderer. Repository [here](https://github.com/cimgui/cimgui/tree/094a55523a40fdb309f48b971a583ef02aeb56ab))
 - VMA (Vulkan memory allocator. Repository [here](https://github.com/CloudCodingSpace/VMA))
