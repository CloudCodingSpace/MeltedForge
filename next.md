# For devs
Stuff to do next (Only next tiny steps to do!) :-

 - change MFArray to delete or insert a element at any idx
 - remove any unnecessary vulkan code from the renderer frontend
 - plan on removing stuffs like mfmesh, mfmodel from the renderer, cuz the renderer should only render.
 - refactor the ecs (needs a separate branch)
    - use uuids instead of ids across builds
    - introduce a separate component called "RenderComponent"
    - "RenderComponent" will contain stuff related to gpu instead of meshes containing gpu related objects
    - integrate "RenderComponent" and change the way ecs currently renders entities
 - change somethings in core so the client can use core in C++