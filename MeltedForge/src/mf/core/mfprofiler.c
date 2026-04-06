#include "mfprofiler.h"

#include <tracy/TracyC.h>

void mfProfilerMarkFrame(void) {
#ifdef MF_ENABLE_PROFILING
    TracyCFrameMark;
#endif        
}

void mfProfilerMarkFrameStart(const char* name) {
#ifdef MF_ENABLE_PROFILING
    TracyCFrameMarkStart(name);
#endif
}

void mfProfilerMarkFrameEnd(const char* name) {
#ifdef MF_ENABLE_PROFILING
    TracyCFrameMarkEnd(name);
#endif
}