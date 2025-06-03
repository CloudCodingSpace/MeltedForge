#include "mfcamera.h"

void default_update(MFCamera* camera, f64 deltaTime, void* userData) {
    MF_ASSERT(camera == mfnull, mfGetLogger(), "The camera handle provided shouldn't be null!");
    const MFWindowConfig* config = mfGetWindowConfig(camera->window);
    b8 moved = false;

    // Key input
    {
        if(mfInputIsKeyPressed(camera->window, MF_KEY_W)) {
            camera->pos = mfVec3Add(camera->pos, mfVec3MulScalar(camera->front, camera->speed * deltaTime));
            moved = true;
        }
        if(mfInputIsKeyPressed(camera->window, MF_KEY_S)) {
            camera->pos = mfVec3Sub(camera->pos, mfVec3MulScalar(camera->front, camera->speed * deltaTime));
            moved = true;
        }
        if(mfInputIsKeyPressed(camera->window, MF_KEY_A)) {
            camera->pos = mfVec3Add(camera->pos, mfVec3MulScalar(camera->right, camera->speed * deltaTime));
            moved = true;
        }
        if(mfInputIsKeyPressed(camera->window, MF_KEY_D)) {
            camera->pos = mfVec3Sub(camera->pos, mfVec3MulScalar(camera->right, camera->speed * deltaTime));
            moved = true;
        }
        if(mfInputIsKeyPressed(camera->window, MF_KEY_SPACE)) {
            camera->pos = mfVec3Add(camera->pos, mfVec3MulScalar(camera->up, camera->speed * deltaTime));
            moved = true;
        }
        if(mfInputIsKeyPressed(camera->window, MF_KEY_LEFT_SHIFT)) {
            camera->pos = mfVec3Sub(camera->pos, mfVec3MulScalar(camera->up, camera->speed * deltaTime));
            moved = true;
        }
    }

    // Mouse update
    {
        if(mfInputIsMBPressed(camera->window, MF_MOUSE_BUTTON_RIGHT)) {
            f64 xpos, ypos;
            mfInputGetMousePos(camera->window, &xpos, &ypos);

            if(camera->firstMouse) {
                mfInputDisableMouse(camera->window);
                camera->lastX = xpos;
                camera->lastY = ypos;
                camera->firstMouse = false;
            }

            f32 xDelta = (xpos - camera->lastX);
            f32 yDelta = (camera->lastY - ypos);

            camera->lastX = xpos;
            camera->lastY = ypos;

            if(xDelta != 0 || yDelta != 0) {
                xDelta *= camera->sensitivity;
                yDelta *= camera->sensitivity;

                camera->yaw += xDelta;
                camera->pitch += yDelta;
                
                if(camera->pitch > 89.9f)
                    camera->pitch = 89.9f;
                if(camera->pitch < -89.9f)
                    camera->pitch = -89.9f;

                MFVec3 front;
                front.x = cos(camera->yaw * MF_DEG2RAD_MULTIPLIER) * cos(camera->pitch * MF_DEG2RAD_MULTIPLIER);
                front.y = sin(camera->pitch * MF_DEG2RAD_MULTIPLIER);
                front.z = sin(camera->yaw * MF_DEG2RAD_MULTIPLIER) * cos(camera->pitch * MF_DEG2RAD_MULTIPLIER);
                
                camera->front = mfVec3Normalize(front);

                moved = true;
            }
        }
        else if(mfInputIsMBReleased(camera->window, MF_MOUSE_BUTTON_RIGHT)) {
            mfInputNormalMouse(camera->window);
            camera->firstMouse = true;
        }
    }

    // Update to the matrices & vectors
    {
        camera->right = mfVec3Normalize(mfVec3Cross((MFVec3){0, 1, 0}, camera->front));
        camera->up = mfVec3Normalize(mfVec3Cross(camera->front, camera->right));
        
        if(!moved)
            return;

        camera->proj = mfMat4Perspective(camera->fov * MF_DEG2RAD_MULTIPLIER, (f32)config->width/(f32)config->height, camera->nearPlane, camera->farPlane);
        camera->view = mfMat4LookAt(camera->pos, mfVec3Add(camera->pos, camera->front), camera->up);
    }
}

void mfCameraCreate(MFCamera* camera, MFWindow* window, f32 fov, f32 nearPlane, f32 farPlane, f32 speed, f32 sensitivity, MFVec3 pos) {
    MF_ASSERT(camera == mfnull, mfGetLogger(), "The camera handle provided shouldn't be null!");    
    MF_ASSERT(window == mfnull, mfGetLogger(), "The window handle provided shouldn't be null!");    
    
    camera->pos = pos;
    camera->window = window;
    camera->fov = fov;
    camera->nearPlane = nearPlane;
    camera->farPlane = farPlane;
    camera->sensitivity = sensitivity;
    camera->speed = speed;
    camera->firstMouse = true;

    camera->front = mfVec3Create(0, 0, -1);
    camera->up = mfVec3Create(0, 1, 0);
    camera->right = mfVec3Normalize(mfVec3Cross(camera->up, camera->front));

    camera->yaw = -90.0f;
    camera->pitch = 0.0f;

    camera->update = &default_update;

    const MFWindowConfig* config = mfGetWindowConfig(window);
    camera->lastX = config->width/2;
    camera->lastY = config->height/2;

    camera->proj = mfMat4Perspective(fov * MF_DEG2RAD_MULTIPLIER, (f32)config->width/(f32)config->height, nearPlane, farPlane);
    camera->view = mfMat4LookAt(pos, mfVec3Add(pos, camera->front), camera->up);
}

void mfCameraDestroy(MFCamera* camera) {
    MF_ASSERT(camera == mfnull, mfGetLogger(), "The camera handle provided shouldn't be null!");    

    MF_SETMEM(camera, 0, sizeof(MFCamera));
}