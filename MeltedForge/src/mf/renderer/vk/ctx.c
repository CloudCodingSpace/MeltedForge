#include "ctx.h"

#include <GLFW/glfw3.h>

void mfVkBckndCtxInit(MFVkBackendCtx* ctx, const char* appName, MFWindow* window) {
    ctx->callbacks = mfnull; // TODO: Create a custom allocator

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

        VK_CHECK(vkCreateInstance(&info, ctx->callbacks, &ctx->instance));
    }
    // Surface
    {
        VK_CHECK(glfwCreateWindowSurface(ctx->instance, mfGetWindowHandle(window), ctx->callbacks, &ctx->surface));
    }
}

void mfVkBckndCtxDestroy(MFVkBackendCtx* ctx) {
    vkDestroySurfaceKHR(ctx->instance, ctx->surface, ctx->callbacks);
    vkDestroyInstance(ctx->instance, ctx->callbacks);

    ctx->callbacks = 0;
    ctx->instance = 0;
}