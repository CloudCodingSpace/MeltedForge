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

void log_fatal(const char* msg) {
    slogLogConsole(&ctx->logger, SLOG_SEVERITY_FATAL, msg);
    abort();
}

void mfInit(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context shouldn't be null!\n");
        abort();
    }
    
    if(ctx->init) {
        slogLogConsole(&ctx->logger, SLOG_SEVERITY_WARN, "The same context is already initialized!\n");
        return;
    }
    
    slogLoggerReset(&ctx->logger);
    slogLoggerSetName(&ctx->logger, "MeltedForge");
    
    ctx->init = true;
}

void mfShutdown(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context shouldn't be null!\n");
        abort();
    }
    
    if(!ctx->init) {
        printf("[MeltedForge]: The current context is not yet initialized!\n");
        abort();
    }

    ctx->init = false;
    glfwTerminate();
}

void mfSetCurrentContext(MFContext* ctx_) {
    if(ctx_ == mfnull) {
        printf("[MeltedForge]: The context provided shouldn't be null!\n");
        abort();
    }
    
    ctx = ctx_;
}

MFContext* mfCheckCurrentContext(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context is null as it is not set!\n");
        abort();
    }
    return ctx;
}

size_t mfGetContextSizeInBytes(void) {
    return sizeof(MFContext);
}

SLogger* mfGetLogger(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context shouldn't be null!\n");
        abort();
    }
    
    return &ctx->logger;
}