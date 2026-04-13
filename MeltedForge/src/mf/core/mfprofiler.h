#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <tracy/TracyC.h>

void mfProfilerMarkFrame(void);

void mfProfilerMarkFrameStart(const char* name);
void mfProfilerMarkFrameEnd(const char* name);

#ifdef MF_ENABLE_PROFILING
    #define MF_PROFILE_ZONE_START(ctx) TracyCZone(ctx, 1)
    #define MF_PROFILE_ZONE_START_NAMED(ctx, name) TracyCZoneN(ctx, name, 1)
    #define MF_PROFILE_ZONE_END(ctx) TracyCZoneEnd(ctx)
#else
    #define MF_PROFILE_ZONE_START(ctx)
    #define MF_PROFILE_ZONE_START_NAMED(ctx, name)
    #define MF_PROFILE_ZONE_END(ctx)
#endif

#ifdef __cplusplus
}
#endif