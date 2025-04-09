#include <mf.h>

#include <stdlib.h>
#include <string.h>

int main(int argc, const char** argv) {
    MFContext* context = MF_ALLOCMEM(MFContext, mfGetContextSizeInBytes());
    
    mfSetCurrentContext(context);
    mfInit();
    
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "Hello from MFTest!");

    mfShutdown();
    MF_FREEMEM(context);
    return 0;
}