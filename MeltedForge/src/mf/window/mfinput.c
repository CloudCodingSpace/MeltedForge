#include "mfinput.h"

#include "core/mfcore.h"

#include <GLFW/glfw3.h>

b8 mfInputIsKeyPressed(MFWindow* window, i32 key) {
    MF_PANIC_IF(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    return glfwGetKey(mfGetWindowHandle(window), key) == GLFW_PRESS;
}

b8 mfInputIsKeyReleased(MFWindow* window, i32 key) {
    MF_PANIC_IF(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    return glfwGetKey(mfGetWindowHandle(window), key) == GLFW_RELEASE;
}

void mfInputGetMousePos(MFWindow* window, f64* x, f64* y) {
    MF_PANIC_IF(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    MF_PANIC_IF(x == mfnull, mfGetLogger(), "The xpos handle shouldn't be null!");
    MF_PANIC_IF(y == mfnull, mfGetLogger(), "The ypos handle shouldn't be null!");
    glfwGetCursorPos(mfGetWindowHandle(window), x, y);
}

void mfInputSetMousePos(MFWindow* window, f64 x, f64 y) {
    MF_PANIC_IF(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    glfwSetCursorPos(mfGetWindowHandle(window), x, y);
}

void mfInputDisableMouse(MFWindow* window) {
    MF_PANIC_IF(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    glfwSetInputMode(mfGetWindowHandle(window), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void mfInputHideMouse(MFWindow* window) {
    MF_PANIC_IF(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    glfwSetInputMode(mfGetWindowHandle(window), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void mfInputNormalMouse(MFWindow* window) {
    MF_PANIC_IF(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    glfwSetInputMode(mfGetWindowHandle(window), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

b8 mfInputIsMBPressed(MFWindow* window, i32 button) {
    MF_PANIC_IF(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    return glfwGetMouseButton(mfGetWindowHandle(window), button) == GLFW_PRESS;
}

b8 mfInputIsMBReleased(MFWindow* window, i32 button) {
    MF_PANIC_IF(window == mfnull, mfGetLogger(), "The window handle shouldn't be null!");
    return glfwGetMouseButton(mfGetWindowHandle(window), button) == GLFW_RELEASE;
}