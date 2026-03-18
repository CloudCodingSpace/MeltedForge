#pragma once

#include "core/mfutils.h"

typedef struct GLFWwindow GLFWwindow;

typedef struct MFWindowConfig_s {
    f64 x, y;
    i32 width, height;
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

const char* mfWindowGetTitle(MFWindow* window);
void mfWindowSetTitle(MFWindow* window, const char* title);

GLFWwindow* mfWindowGetHandle(MFWindow* window);
const MFWindowConfig* mfWindowGetConfig(MFWindow* window);

size_t mfWindowGetSizeInBytes(void);