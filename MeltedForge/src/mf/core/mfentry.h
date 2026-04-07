#pragma once

#include "mfapp.h"

MFAppConfig mfClientCreateAppConfig();

int main(int argc, const char** argv) {
    mfInitialize();
    MFAppConfig config = mfClientCreateAppConfig();
    
    if(config.initApp)
        config.initApp(config.state, &config);

    if(config.runApp)
        config.runApp(config.state, &config);

    if(config.shutdownApp)
        config.shutdownApp(config.state, &config);

    mfShutdown();
    return 0;
}
