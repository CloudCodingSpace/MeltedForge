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

f64 mfGetCurrentTime();