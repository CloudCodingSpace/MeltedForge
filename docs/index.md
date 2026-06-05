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


 - A Basic ECS
   - Multiple entities support
   - Can add/remove components to/from a scene
   - Can attach/detach components from entities
   - Basic scene management
   - Scene load/save
   - Material system
 - GLTF 2.0 model loading along with the material data
    - Can load complex models (Tested with Sponza & Bistro Internal)
 - Engine & editor level UI
    - UI customization (Using Dear ImGui's styles)
 - Render targets
    - Objects with functionality to set the render output to an image, which can be used to render the scene inside an UI panel like the scene viewport
    - In other words, they are offscreen framebuffers on which we can render something onto
 - A binary serialization/deserialization api
 - Explicit gpu resource management control for resources like:
    - Vertex buffers
    - Index buffers
    - Textures and skyboxes
    - UBOs
    - SSBOs
    - Image samplers (`sampler2D` and `samplerCube` in glsl)
    - ResourceSets and ResourceSetLayouts (Custom engine type)
 - Skybox support
    - Only 2D equirectangular images as input
    - HDR format supported as well
    - Optional to generate utility IBL textures such as BRDF LUT, Prefiltered Map and Irradiance map
 - Basic PBR support
    - Diffuse IBL support
    - Specular IBL support
 - MSAA support for native window and render targets as well

---

## Third party dependencies

The engine uses a few third party dependencies internally. They are:

 - Slog (Custom logger by the same author. Repository [here](https://github.com/CloudCodingSpace/slog))
 - Stb's single header utils (Single header utils by stb. Repository [here](https://github.com/CloudCodingSpace/stb))
 - Glfw (Cross-platform windowing/util library. Repository [here](https://github.com/CloudCodingSpace/GLFW/tree/73f09546ccd681047360e4e8bc5474adad55147f))
 - Tracy (For profiling. Repository [here](https://github.com/wolfpld/tracy/tree/05cceee0df3b8d7c6fa87e9638af311dbabc63cb))
 - Assimp (Model loader. Repository [here](https://github.com/assimp/assimp/tree/e0b52347c6e52de2827ec957a9ebf00ce3c54f79))
 - CImGui (C bindings for Dear ImGui, a immediate mode UI renderer. Repository [here](https://github.com/cimgui/cimgui/tree/094a55523a40fdb309f48b971a583ef02aeb56ab))