#pragma once

#include "core/mfutils.h"

typedef struct GLFWwindow GLFWwindow;

typedef struct MFWindowConfig_s {
    i32 x, y, width, height;
    b8 fullscreen, resizable, centered;
    const char* title;
} MFWindowConfig;

#define MF_WINDOW_DEFAULT_ICON_PATH "mfassets/logo/logo2.png"

typedef struct MFWindow_s MFWindow;

void mfWindowInit(MFWindow* window, MFWindowConfig config);
void mfWindowDestroy(MFWindow* window);

void mfWindowSetIcon(MFWindow* window, const char* path);

void mfWindowUpdate(MFWindow* window);
void mfWindowClose(MFWindow* window);

void mfWindowShow(MFWindow* window);
void mfWindowHide(MFWindow* window);
b8 mfIsWindowOpen(MFWindow* window);

const char* mfGetWindowTitle(MFWindow* window);
void mfSetWindowTitle(MFWindow* window, const char* title);

GLFWwindow* mfGetWindowHandle(MFWindow* window);
const MFWindowConfig* mfGetWindowConfig(MFWindow* window);

size_t mfWindowGetSizeInBytes(void);