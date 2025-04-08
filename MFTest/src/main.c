#include <mf.h>

#include <stdlib.h>
#include <string.h>

int main(int argc, const char** argv) {
    MFContext* context = (MFContext*) malloc(mfGetContextSizeInBytes());
    memset(context, 0, mfGetContextSizeInBytes());
    
    mfSetCurrentContext(context);
    mfInit("MFTest");
    
    slogLogConsole(mfGetLogger(), SLOG_SEVERITY_INFO, "Hello from MFTest!");

    mfShutdown();
    free(context);
    return 0;
}