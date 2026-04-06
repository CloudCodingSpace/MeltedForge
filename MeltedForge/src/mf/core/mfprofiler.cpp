#include "mfprofiler.h"

#include <tracy/Tracy.hpp>

extern "C" {

    void mfProfilerMarkFrame(void) {
        FrameMark;
    }

    void mfProfilerMarkFrameStart(const char* name) {
        FrameMarkStart(name);
    }

    void mfProfilerMarkFrameEnd(const char* name) {
        FrameMarkEnd(name);
    }

    void mfProfilerZoneScoped(void) {
        
    }

}