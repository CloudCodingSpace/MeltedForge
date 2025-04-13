#include "mfinput.h"

#include "core/mfcore.h"

#include <GLFW/glfw3.h>

b8 mfInputIsKeyPressed(MFWindow* window, f32 key) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    return glfwGetKey(mfGetWindowHandle(window), key) == GLFW_PRESS;
}

b8 mfInputIsKeyReleased(MFWindow* window, f32 key) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    return glfwGetKey(mfGetWindowHandle(window), key) == GLFW_RELEASE;
}

void mfInputGetMousePos(MFWindow* window, f64* x, f64* y) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    MF_ASSERT(x == mfnull, mfGetLogger(), "The xpos handle shouldn't be null!");
    MF_ASSERT(y == mfnull, mfGetLogger(), "The ypos handle shouldn't be null!");
    glfwGetCursorPos(mfGetWindowHandle(window), x, y);
}

void mfInputSetMousePos(MFWindow* window, f64 x, f64 y) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    glfwSetCursorPos(mfGetWindowHandle(window), x, y);
}

void mfInputDisableMouse(MFWindow* window) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    glfwSetInputMode(mfGetWindowHandle(window), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void mfInputHideMouse(MFWindow* window) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    glfwSetInputMode(mfGetWindowHandle(window), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void mfInputNormalMouse(MFWindow* window) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    glfwSetInputMode(mfGetWindowHandle(window), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

b8 mfInputIsMBPressed(MFWindow* window, f32 button) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    return glfwGetMouseButton(mfGetWindowHandle(window), button) == GLFW_PRESS;
}

b8 mfInputIsMBReleased(MFWindow* window, f32 button) {
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    return glfwGetMouseButton(mfGetWindowHandle(window), button) == GLFW_RELEASE;
}