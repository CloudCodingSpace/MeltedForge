#include "mfprofiler.h"

#include <tracy/Tracy.hpp>

extern "C" {

    void mfProfilerMarkFrame(void) {
#ifdef MF_ENABLE_PROFILING
        FrameMark;
#endif
    }

    void mfProfilerMarkFrameStart(const char* name) {
#ifdef MF_ENABLE_PROFILING
        FrameMarkStart(name);
#endif
    }

    void mfProfilerMarkFrameEnd(const char* name) {
#ifdef MF_ENABLE_PROFILING
        FrameMarkEnd(name);
#endif
    }

    void mfProfilerZoneScoped(void) {
#ifdef MF_ENABLE_PROFILING
        ZoneScoped;
#endif
    }

}