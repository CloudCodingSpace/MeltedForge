#pragma once

#include "window/mfwindow.h"

#include "ctx.h"

typedef struct MFVkBackend_s {
    MFVkBackendCtx ctx;
} MFVkBackend;

void mfVkBckndInit(MFVkBackend* backend, const char* appName, MFWindow* window);
void mfVkBckndShutdown(MFVkBackend* backend);

void mfVkBckndBeginframe(MFVkBackend* backend);
void mfVkBckndEndframe(MFVkBackend* backend);