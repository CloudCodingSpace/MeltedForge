#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mfutils.h"

typedef struct MFTimer_s {
    f64 start;
    f64 end;
    f64 delta;
    bool started;
} MFTimer;

// @brief Starts the timer
// @param timer A valid non-null pointer to a `MFTimer`
void mfTimerStart(MFTimer* timer);

// @brief Stops the timer
// @param timer A valid non-null pointer to a `MFTimer`
void mfTimerEnd(MFTimer* timer);

// @brief Resets the timer
// @param timer A valid non-null pointer to a `MFTimer`
void mfTimerReset(MFTimer* timer);

// @brief Gets the elapsed time since the engine was initialised
f64 mfGetTimeElapsed(void);

// @brief Gets current second
f64 mfGetCurrentTimeSecs(void);

// @brief Gets the current time in minutes
u64 mfGetCurrentTimeMins(void);

// @brief Gets the current hour
u64 mfGetCurrentTimeHours(void);

// @brief Gets the current day
u64 mfGetCurrentTimeDays(void);

// @brief Gets the current month
u32 mfGetCurrentTimeMonths(void);

// @brief Gets the current year
u64 mfGetCurrentTimeYears(void);

#ifdef __cplusplus
}
#endif