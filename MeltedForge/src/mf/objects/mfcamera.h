#pragma once

#include "window/mfwindow.h"
#include "window/mfinput.h"

#include "core/mfmaths.h"

typedef struct MFCamera_s {
    MFMat4 proj, view;
    MFVec3 front, up, right, pos;
    f32 width, height, yaw, pitch, fov, nearPlane, farPlane, speed, sensitivity, lastX, lastY;
    b8 firstMouse;
    MFWindow* window;

    void (*update)(struct MFCamera_s* camera, f64 deltaTime, void* userData);
} MFCamera;

void mfCameraCreate(MFCamera* camera, MFWindow* window, f32 width, f32 height, f32 fov, f32 nearPlane, f32 farPlane, f32 speed, f32 sensitivity, MFVec3 pos);
void mfCameraDestroy(MFCamera* camera);