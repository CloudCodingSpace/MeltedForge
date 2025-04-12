#pragma once

#include "mfapp.h"

MFAppConfig mfClientCreateAppConfig();

int main(int argc, const char** argv) {
    MFAppConfig config = mfClientCreateAppConfig();
    MFContext* context = MF_ALLOCMEM(MFContext, mfGetContextSizeInBytes());

    mfSetCurrentContext(context);
    mfInit();
    
    if(config.initApp)
        config.initApp(config.state, &config);

    if(config.runApp)
        config.runApp(config.state, &config);

    if(config.shutdownApp)
        config.shutdownApp(config.state, &config);

    mfShutdown();

    MF_FREEMEM(context);
    return 0;
}