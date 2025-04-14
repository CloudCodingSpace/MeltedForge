#include "backend.h"

#include <GLFW/glfw3.h>

void mfVkBckndInit(MFVkBackend* backend, const char* appName, MFWindow* window) {
    mfVkBckndCtxInit(&backend->ctx, appName, window);
}

void mfVkBckndShutdown(MFVkBackend* backend) {
    mfVkBckndCtxDestroy(&backend->ctx);
}

void mfVkBckndBeginframe(MFVkBackend* backend) {

}

void mfVkBckndEndframe(MFVkBackend* backend) {

}