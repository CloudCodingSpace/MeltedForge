#include "mfcore.h"
#include "mfutils.h"

#include <slog/slog.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

struct MFContext_s {
    SLogger logger;
    bool init;
};

static MFContext* ctx = mfnull;

void mfInit(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context shouldn't be null!");
        abort();
    }
    
    if(ctx->init) {
        slogLogMsg(&ctx->logger, SLOG_SEVERITY_WARN, "The same context is already initialized!");
        return;
    }
    
    slogLoggerCreate(&ctx->logger, "MeltedForge", mfnull, SLOG_LOGGER_FEATURE_LOG2CONSOLE); // TODOD: Make the features configurable!!
    
    ctx->init = true;
}

void mfShutdown(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context shouldn't be null!");
        abort();
    }
    
    if(!ctx->init) {
        printf("[MeltedForge]: The current context is not yet initialized!");
        abort();
    }

    slogLoggerDestroy(&ctx->logger);

    ctx->init = false;
    glfwTerminate();
}

void mfSetCurrentContext(MFContext* ctx_) {
    if(ctx_ == mfnull) {
        printf("[MeltedForge]: The context provided shouldn't be null!");
        abort();
    }
    
    ctx = ctx_;
}

MFContext* mfGetCurrentContext(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context is null as it is not set!");
        abort();
    }
    return ctx;
}

size_t mfGetContextSizeInBytes(void) {
    return sizeof(MFContext);
}

SLogger* mfGetLogger(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context shouldn't be null!");
        abort();
    }
    
    return &ctx->logger;
}
