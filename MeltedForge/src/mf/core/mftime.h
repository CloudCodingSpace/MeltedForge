#pragma once

#include "mfutils.h"

typedef struct MFTimer_s {
    f64 start;
    f64 end;
    f64 delta;
    b8 started;
} MFTimer;

void mfTimerStart(MFTimer* timer);
void mfTimerEnd(MFTimer* timer);
void mfTimerReset(MFTimer* timer);

f64 mfGetTimeElapsed(void);
f64 mfGetCurrentTimeSecs(void);
u64 mfGetCurrentTimeMins(void);
u64 mfGetCurrentTimeHours(void);
u64 mfGetCurrentTimeDays(void);
u32 mfGetCurrentTimeMonths(void);
u64 mfGetCurrentTimeYears(void);