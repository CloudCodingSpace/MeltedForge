#include "mfcore.h"
#include "mfutils.h"

#include "serializer/mfserializer.h"
#include "serializer/mfserializerutils.h"

#include <slog/slog.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

struct MFContext_s {
    SLogger logger;
    bool init;
    // UUID Generator
    u32 sessionID;
    u32 counter;
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
    
    slogLoggerCreate(&ctx->logger, "MeltedForge", mfnull, SLOG_LOGGER_FEATURE_LOG2CONSOLE); // TODO: Make the features configurable!!

    // Deserializing
    do {
        FILE* file = fopen("mfcore_cache.bin", "rb");
        if(file == mfnull)
            break;

        fseek(file, 0, SEEK_END);
        u64 size = ftell(file);
        fseek(file, 0, SEEK_SET);

        u8* content = MF_ALLOCMEM(u8, size);
        fread(content, 1, size, file);

        MFSerializer serializer = {
            .buffer = content
        };
        mfSerializerCreate(&serializer, size, true);
        
        u32 sign = mfDeserializeU32(&serializer);
        if(sign != MF_SIGNATURE_CORE_CACHE_FILE) {
            mfSerializerDestroy(&serializer);
            fclose(file);
            break;
        }

        ctx->sessionID = mfDeserializeU32(&serializer);
        ctx->counter = mfDeserializeU32(&serializer);

        mfSerializerDestroy(&serializer);
        fclose(file);
    } while(0);

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

    // Serializing
    {
        MFSerializer serializer = {0};
        mfSerializerCreate(&serializer, sizeof(u32) * 3, false);
        
        mfSerializeU32(&serializer, MF_SIGNATURE_CORE_CACHE_FILE);
        mfSerializeU32(&serializer, ctx->sessionID);
        mfSerializeU32(&serializer, ctx->counter);

        do {
            FILE* file = fopen("mfcore_cache.bin", "wb");
            if(file == mfnull) {
                slogLogMsg(&ctx->logger, SLOG_SEVERITY_ERROR, "Can't serialize to core cache file named 'mfcore_cache.bin'!");
                break;
            }

            fwrite(serializer.buffer, serializer.bufferSize, 1, file);

            fclose(file);
        } while(0);

        mfSerializerDestroy(&serializer);
    }

    slogLoggerDestroy(&ctx->logger);

    ctx->init = false;
    glfwTerminate();
}

u64 mfGetNextID(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context shouldn't be null!");
        abort();
    }
    
    if(!ctx->init) {
        printf("[MeltedForge]: The current context is not yet initialized!");
        abort();
    }
    
    if(ctx->counter == UINT32_MAX) {
        if(ctx->sessionID == UINT32_MAX) {
            slogLogMsg(&ctx->logger, SLOG_SEVERITY_FATAL, "[MeltedForge]: Can't generate more IDs!");
            abort();
        }
        ctx->sessionID++;
        ctx->counter = 0;
    }

    return (u64)(((u64)ctx->sessionID << 32) | (ctx->counter++));
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
