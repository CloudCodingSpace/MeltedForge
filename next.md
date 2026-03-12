# For devs
Stuff to do next (Only next tiny steps to do!) :-

 - plan on removing stuffs like mfmesh, mfmodel from the renderer, cuz the renderer should only render.
 - dont let the client worry about FRAMES_IN_FLIGHT during ubo creation
 - refactor the ecs (needs a separate branch)
    - use uuids instead of ids across builds
    - introduce a separate component called "RenderComponent"
    - "RenderComponent" will contain stuff related to gpu instead of meshes containing gpu related objects
    - integrate "RenderComponent" and change the way ecs currently renders entities
 - change somethings in core so the client can use core in C++