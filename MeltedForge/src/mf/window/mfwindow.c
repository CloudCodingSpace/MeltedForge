#include "mfwindow.h"

#include "core/mfcore.h"

#include <GLFW/glfw3.h>

#include <stb/stb_image.h>

struct MFWindow_s {
    GLFWwindow* handle;
    MFWindowConfig config;
    b8 init;
};

void mfWindowInit(MFWindow* window, MFWindowConfig config) {
    mfCheckCurrentContext();

    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!\n");
    }

    if(window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle is already initialized!\n");
    }

    if(!glfwInit()) {
        MF_FATAL_ABORT(mfGetLogger(), "Failed to initialize the system for creating the window!\n");
    }

    glfwWindowHint(GLFW_RESIZABLE, config.resizable);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWmonitor* monitor = mfnull;
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	if(config.fullscreen) {
		monitor = glfwGetPrimaryMonitor();

		config.width = mode->width;
		config.height = mode->height;
		window->config = config;
	}

    window->handle = glfwCreateWindow(config.width, config.height, config.title, monitor, mfnull);
    if(window->handle == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "Failed to create the window!\n");
    }
    window->config.title = glfwGetWindowTitle(window->handle);
    glfwMakeContextCurrent(window->handle);

    glfwSetWindowPos(window->handle, config.x, config.y);

    if(config.centered) {
        glfwSetWindowPos(window->handle, (mode->width - config.width)/2, (mode->height - config.height)/2);
    }

    window->config = config;
    window->init = true;

    MF_INFO(mfGetLogger(), "Created MFWindow\n");
}

void mfWindowDestroy(MFWindow* window) {
    mfCheckCurrentContext();

    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!\n");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle can't be destroyed because it is not initialized!\n");
    }

    glfwDestroyWindow(window->handle);
    MF_SETMEM(window, 0, sizeof(*window));

    MF_INFO(mfGetLogger(), "Destroyed MFWindow\n");
}

void mfWindowSetIcon(MFWindow* window, const char* path) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!\n");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!\n");
    }

    i32 channels;
    GLFWimage img[1];
    MF_SETMEM(img, 0, sizeof(GLFWimage));
    u8* data = stbi_load(path, &img->width, &img->height, &channels, 4);
    img->pixels = data;
    glfwSetWindowIcon(window->handle, 1, img);
    stbi_image_free(data);

    MF_INFO(mfGetLogger(), "Set an icon to MFWindow\n");
}

void mfWindowUpdate(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!\n");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!\n");
    }

    glfwPollEvents();
}

void mfWindowClose(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!\n");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!\n");
    }
    glfwSetWindowShouldClose(window->handle, true);
    MF_INFO(mfGetLogger(), "Closed MFWindow\n");
}

void mfWindowShow(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!\n");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!\n");
    }
    glfwShowWindow(window->handle);
}

void mfWindowHide(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!\n");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!\n");
    }
    glfwHideWindow(window->handle);
}

b8 mfIsWindowOpen(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!\n");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!\n");
    }
    return !glfwWindowShouldClose(window->handle);
}

GLFWwindow* mfGetWindowHandle(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!\n");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!\n");
    }
    return window->handle;
}

const MFWindowConfig* mfGetWindowConfig(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!\n");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!\n");
    }
    return &window->config;
}

size_t mfWindowGetSizeInBytes(void) {
    return sizeof(MFWindow);
}