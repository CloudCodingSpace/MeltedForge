#include "mftimer.h"

#include "mfcore.h"
#include <GLFW/glfw3.h>

void mfTimerStart(MFTimer* timer) {
    if(timer == mfnull) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The timer should be a valid pointer to a MFTimer!\n");
        return;
    }

    if(timer->started) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The timer is not yet ended for the next start!\n");
        return;
    }

    timer->start = glfwGetTime();
    timer->end = 0;
    timer->started = true;
}

void mfTimerEnd(MFTimer* timer) {
    if(timer == mfnull) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The timer should be a valid pointer to a MFTimer!\n");
        return;
    }

    if(!timer->started) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The timer is not yet started to be ended!\n");
        return;
    }

    timer->end = glfwGetTime();
    timer->delta = timer->end - timer->start;
    timer->delta *= 1000; // seconds to ms
    timer->started = false;
}

void mfTimerReset(MFTimer* timer) {
    if(timer == mfnull) {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_WARN, "The timer should be a valid pointer to a MFTimer!\n");
        return;
    }

    timer->delta = timer->end = timer->start = 0;
    timer->started = false;
}