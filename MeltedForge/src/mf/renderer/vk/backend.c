#include "backend.h"

#include "common.h"

#include <GLFW/glfw3.h>

void mfVkBckndInit(MFVkBackend* backend, const char* appName, MFWindow* window) {
    backend->callbacks = mfnull; // TODO: Create a custom allocator

    // Vulkan instance
    {
        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pApplicationName = appName,
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "MeltedForge",
            .apiVersion = VK_API_VERSION_1_0
        };

        u32 extCount = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&extCount);

        VkInstanceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = extCount,
            .ppEnabledExtensionNames = exts
        };

        VK_CHECK(vkCreateInstance(&info, backend->callbacks, &backend->instance));
    }
    // Surface
    {
        VK_CHECK(glfwCreateWindowSurface(backend->instance, mfGetWindowHandle(window), backend->callbacks, &backend->surface));
    }
}

void mfVkBckndShutdown(MFVkBackend* backend) {
    vkDestroySurfaceKHR(backend->instance, backend->surface, backend->callbacks);
    vkDestroyInstance(backend->instance, backend->callbacks);

    backend->callbacks = 0;
    backend->instance = 0;
}

void mfVkBckndBeginframe(MFVkBackend* backend) {

}

void mfVkBckndEndframe(MFVkBackend* backend) {

}