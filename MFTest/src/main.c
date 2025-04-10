#include <mf.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, const char** argv) {
    MFContext* context = MF_ALLOCMEM(MFContext, mfGetContextSizeInBytes());
    MFWindow* window = MF_ALLOCMEM(MFWindow, mfWindowGetSizeInBytes());

    mfSetCurrentContext(context);
    mfInit();

    mfWindowInit(window, (MFWindowConfig){
        .centered = true,
        .fullscreen = false,
        .resizable = false,
        .title = "MFTest",
        .width = 800,
        .height = 600
    });

    mfWindowShow(window);

    while(mfIsWindowOpen(window)) {
        // Rendering and other stuff

        mfWindowUpdate(window);
    }

    mfWindowDestroy(window);
    mfShutdown();

    MF_FREEMEM(window);
    MF_FREEMEM(context);
    return 0;
}