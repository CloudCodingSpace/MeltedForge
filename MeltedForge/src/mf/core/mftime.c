#ifdef __cplusplus
extern "C" {
#endif

#include "mftime.h"

#include "mfcore.h"
#include <GLFW/glfw3.h>
#include <time.h>

void mfTimerStart(MFTimer* timer) {
    if(timer == mfnull) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The timer should be a valid pointer to a MFTimer!");
        return;
    }

    if(timer->started) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The timer is not yet ended for the next start!");
        return;
    }

    timer->start = glfwGetTime();
    timer->end = 0;
    timer->started = true;
}

void mfTimerEnd(MFTimer* timer) {
    if(timer == mfnull) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The timer should be a valid pointer to a MFTimer!");
        return;
    }

    if(!timer->started) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The timer is not yet started to be ended!");
        return;
    }

    timer->end = glfwGetTime();
    timer->delta = timer->end - timer->start;
    timer->delta *= 1000; // seconds to ms
    timer->started = false;
}

void mfTimerReset(MFTimer* timer) {
    if(timer == mfnull) {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The timer should be a valid pointer to a MFTimer!");
        return;
    }

    timer->delta = timer->end = timer->start = 0;
    timer->started = false;
}

f64 mfGetTimeElapsed(void) {
    return glfwGetTime();
}

f64 mfGetCurrentTimeSecs(void) {
    time_t now = time(mfnull);
    struct tm* t = localtime(&now);
    return t->tm_sec;
}

u64 mfGetCurrentTimeMins(void) {
    time_t now = time(mfnull);
    struct tm* t = localtime(&now);
    return t->tm_min;
    
}

u64 mfGetCurrentTimeHours(void) {
    time_t now = time(mfnull);
    struct tm* t = localtime(&now);
    return t->tm_hour;
}

u64 mfGetCurrentTimeDays(void) {
    time_t now = time(mfnull);
    struct tm* t = localtime(&now);
    return t->tm_mday;
}

u32 mfGetCurrentTimeMonths(void) {
    time_t now = time(mfnull);
    struct tm* t = localtime(&now);
    return t->tm_mon + 1;
}

u64 mfGetCurrentTimeYears(void) {
    time_t now = time(mfnull);
    struct tm* t = localtime(&now);
    return t->tm_year + 1900;
}

#ifdef __cplusplus
}
#endif