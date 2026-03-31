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

    u32 errorImageWidth;
    u32 errorImageHeight;
    u8* errorImagePixels;
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
        b8 success = false;
        u64 size;
        char* content = mfReadFile(mfGetLogger(), &size, &success, "mfcore_cache.bin", "rb");
        MF_DO_IF(!success, {
            slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to open the file! Most probably because the file doesn't exist or the reading mode is wrong!");
            break;
        });

        MFSerializer serializer = {
            .buffer = content
        };
        mfSerializerCreate(&serializer, size, true);
        
        u32 sign = mfDeserializeU32(&serializer);
        if(sign != MF_SIGNATURE_CORE_CACHE_FILE) {
            mfSerializerDestroy(&serializer);
            break;
        }

        ctx->sessionID = mfDeserializeU32(&serializer);
        ctx->counter = mfDeserializeU32(&serializer);

        mfSerializerDestroy(&serializer);
    } while(0);

    // Error texture
    {
        ctx->errorImageWidth = 4;
        ctx->errorImageHeight = 4;
        ctx->errorImagePixels = MF_ALLOCMEM(u8, ctx->errorImageWidth * ctx->errorImageHeight * 4);
        u32 cellSize = 1;
        for(u32 h = 0; h < 4; h++) {
            for(u32 w = 0; w < 4; w++) {
                u32 idx = (h * 4 + w) * 4;
                u32 x = w / cellSize;
                u32 y = h / cellSize;

                if(((x + y) % 2) == 0) {
                    ctx->errorImagePixels[idx + 0] = 0xff;
                    ctx->errorImagePixels[idx + 1] = 0x00;
                    ctx->errorImagePixels[idx + 2] = 0xff;
                    ctx->errorImagePixels[idx + 3] = 0xff;
                } else {
                    ctx->errorImagePixels[idx + 0] = 0x00;
                    ctx->errorImagePixels[idx + 1] = 0x00;
                    ctx->errorImagePixels[idx + 2] = 0x00;
                    ctx->errorImagePixels[idx + 3] = 0xff;
                }
            }
        }
    }

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
            b8 success = mfWriteFile(mfGetLogger(), serializer.bufferSize, "mfcore_cache.bin", serializer.buffer, "wb");
            MF_DO_IF(!success, {
                slogLogMsg(mfGetLogger(), SLOG_SEVERITY_ERROR, "Failed to open the file! Most probably because the file doesn't exist or the reading mode is wrong!");
                break;
            });
        } while(0);

        mfSerializerDestroy(&serializer);
    }

    MF_FREEMEM(ctx->errorImagePixels);
    ctx->errorImageWidth = 0;
    ctx->errorImageHeight = 0;

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

u8* mfGetErrorImagePixels(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context shouldn't be null!");
        abort();
    }
    
    if(!ctx->init) {
        printf("[MeltedForge]: The current context is not yet initialized!");
        abort();
    }
    
    return ctx->errorImagePixels;
}

u32 mfGetErrorImageWidth(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context shouldn't be null!");
        abort();
    }
    
    if(!ctx->init) {
        printf("[MeltedForge]: The current context is not yet initialized!");
        abort();
    }
    
    return ctx->errorImageWidth;
}

u32 mfGetErrorImageHeight(void) {
    if(ctx == mfnull) {
        printf("[MeltedForge]: The current context shouldn't be null!");
        abort();
    }
    
    if(!ctx->init) {
        printf("[MeltedForge]: The current context is not yet initialized!");
        abort();
    }

    return ctx->errorImageHeight;
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
