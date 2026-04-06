#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void mfProfilerMarkFrame(void);

void mfProfilerMarkFrameStart(const char* name);
void mfProfilerMarkFrameEnd(const char* name);

void mfProfilerZoneScoped(void);

#ifdef __cplusplus
}
#endif