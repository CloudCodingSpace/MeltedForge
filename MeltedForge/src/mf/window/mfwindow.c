#include "mfwindow.h"

#include "core/mfcore.h"

#include <GLFW/glfw3.h>

#include <stb/stb_image.h>

struct MFWindow_s {
    GLFWwindow* handle;
    MFWindowConfig config;
    b8 init;
};

static void size_callback(GLFWwindow* window, int x, int y) {
    MFWindow* win = (MFWindow*)glfwGetWindowUserPointer(window);

    win->config.width = x;
    win->config.height = y;
}

static void pos_callback(GLFWwindow* window, double x, double y) {
    MFWindow* win = (MFWindow*)glfwGetWindowUserPointer(window);

    win->config.x = x;
    win->config.y = y;
}

void mfWindowInit(MFWindow* window, MFWindowConfig config) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }

    if(window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle is already initialized!");
    }

    if(!glfwInit()) {
        MF_FATAL_ABORT(mfGetLogger(), "Failed to initialize the system for creating the window!");
    }

    glfwWindowHint(GLFW_RESIZABLE, config.resizable);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

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
        MF_FATAL_ABORT(mfGetLogger(), "Failed to create the window!");
    }
    window->config.title = glfwGetWindowTitle(window->handle);
    glfwMakeContextCurrent(window->handle);
    glfwSetWindowUserPointer(window->handle, window);
    glfwSetFramebufferSizeCallback(window->handle, size_callback);
    glfwSetCursorPosCallback(window->handle, pos_callback);

    glfwSetWindowPos(window->handle, config.x, config.y);

    if(config.centered) {
        glfwSetWindowPos(window->handle, (mode->width - config.width)/2, (mode->height - config.height)/2);
    }

    window->config = config;
    window->init = true;

    MF_INFO(mfGetLogger(), "Creating MFWindow");
}

void mfWindowDestroy(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle can't be destroyed because it is not initialized!");
    }

    glfwDestroyWindow(window->handle);
    MF_SETMEM(window, 0, sizeof(MFWindow));

    MF_INFO(mfGetLogger(), "Destroying MFWindow");
}

void mfWindowSetIcon(MFWindow* window, const char* path) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!");
    }

    i32 channels;
    GLFWimage img[1];
    MF_SETMEM(img, 0, sizeof(GLFWimage));
    u8* data = stbi_load(path, &img->width, &img->height, &channels, 4);
    img->pixels = data;
    glfwSetWindowIcon(window->handle, 1, img);
    stbi_image_free(data);

    MF_INFO(mfGetLogger(), "Set an icon to MFWindow");
}

void mfWindowUpdate(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!");
    }

    glfwPollEvents();
}

void mfWindowClose(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!");
    }
    glfwSetWindowShouldClose(window->handle, true);
    MF_INFO(mfGetLogger(), "Closed MFWindow");
}

void mfWindowShow(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!");
    }
    glfwShowWindow(window->handle);
}

void mfWindowHide(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!");
    }
    glfwHideWindow(window->handle);
}

b8 mfIsWindowOpen(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!");
    }
    return !glfwWindowShouldClose(window->handle);
}

const char* mfGetWindowTitle(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!");
    }

    return glfwGetWindowTitle(window->handle);
}

void mfSetWindowTitle(MFWindow* window, const char* title) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!");
    }
    if(title == mfnull) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The title that is to be set shouldn't be null!");
        return;
    }

    glfwSetWindowTitle(window->handle, title);
}

GLFWwindow* mfGetWindowHandle(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!");
    }
    return window->handle;
}

const MFWindowConfig* mfGetWindowConfig(MFWindow* window) {
    if(window == mfnull) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle provided shouldn't be null!");
    }
    if(!window->init) {
        MF_FATAL_ABORT(mfGetLogger(), "The window handle should be initialized!");
    }
    return &window->config;
}

size_t mfWindowGetSizeInBytes(void) {
    return sizeof(MFWindow);
}
