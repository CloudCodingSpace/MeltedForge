#pragma once

#include "window/mfwindow.h"

typedef struct MFAppConfig_s {
    void* state;
    MFWindowConfig winConfig;
    void (*initApp)(void* state, MFWindowConfig*);
    void (*shutdownApp)(void* state);
    void (*runApp)(void* state);
} MFAppConfig;

MFAppConfig mfCreateDefaultApp();