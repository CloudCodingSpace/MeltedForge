# About MeltedForge

MeltedForge is a graphics engine or actually a renderer that abstracts API specific code of APIs like Vulkan, that has a very deep learning curve.
MeltedForge is created so that developers don't need to create a renderer backend for each of the API that it uses. It is currently under development.
Now developers just need to tell MeltedForge to render without taking any graphics API's backend into account. Example of a very tiny program :- 

    #include <mf.h>

    int main(int argc, const char** argv) {
        ... // Window creation stuff
        
        mfInitialize(MFBCKND_TYPE_VULKAN); // The API backend that is to be used, has to be mentioned!

        while(isWindowOpen) { // Main loop
            ... // blah blah

            ... // Rendering commands given to MeltedForge

            ... // blah blah            
        }

        mfShutdown();
        ... // Other clenup stuff
    }


# MeltedForge's features
 - For now, the vulkan backend is being written

# TODO
 - Support OpenGL & many other APIs

# MeltedForge's Dependencies (For it's core)
 - VulkanSDK
 - cglm (glm for c)
 - slog (A logger written by me in pure C without any external dependencies)
 - GLAD
 - stb's libraries like stb_image.h
