#ifdef __cplusplus
extern "C" {
#endif

#include "mfcore.h"
#include "mfutils.h"

#include "serializer/mfserializer.h"
#include "serializer/mfserializerutils.h"

#include <slog/slog.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>

typedef struct MFContext_s {
    SLogger logger;
    b8 init;
    // UUID Generator
    u64 counter;

    u32 errorImageWidth;
    u32 errorImageHeight;
    u8* errorImagePixels;
} MFContext;

static MFContext s_Ctx = {0};

void mfInitialize(void) {
    if(s_Ctx.init) {
        slogLogMsg(&s_Ctx.logger, SLOG_SEVERITY_ERROR, "MeltedForge's core is already initialized!");
        abort();
    }
    s_Ctx.init = true;
    
    slogLoggerCreate(&s_Ctx.logger, "MeltedForge", mfnull, SLOG_LOGGER_FEATURE_LOG2CONSOLE); // TODO: Make the features configurable!!

    // Deserializing
    {
        b8 success = false;
        u64 size;
        char* content = mfReadFile(mfGetLogger(), &size, &success, "mfcore_cache.bin", "rb");
        if(success) {
            MFSerializer serializer = {
                .buffer = content
            };
            mfSerializerCreate(&serializer, size, true);
            
            u32 sign = mfDeserializeU32(&serializer);
            if(sign != MF_SIGNATURE_CORE_CACHE_FILE) {
                mfSerializerDestroy(&serializer);
            } else {
                s_Ctx.counter = mfDeserializeU32(&serializer);
                mfSerializerDestroy(&serializer);
            }
        }
    }

    // Error texture
    {
        s_Ctx.errorImageWidth = 4;
        s_Ctx.errorImageHeight = 4;
        s_Ctx.errorImagePixels = MF_ALLOCMEM(u8, s_Ctx.errorImageWidth * s_Ctx.errorImageHeight * 4);
        u32 cellSize = 1;
        for(u32 h = 0; h < 4; h++) {
            for(u32 w = 0; w < 4; w++) {
                u32 idx = (h * 4 + w) * 4;
                u32 x = w / cellSize;
                u32 y = h / cellSize;

                if(((x + y) % 2) == 0) {
                    s_Ctx.errorImagePixels[idx + 0] = 0xff;
                    s_Ctx.errorImagePixels[idx + 1] = 0x00;
                    s_Ctx.errorImagePixels[idx + 2] = 0xff;
                    s_Ctx.errorImagePixels[idx + 3] = 0xff;
                } else {
                    s_Ctx.errorImagePixels[idx + 0] = 0x00;
                    s_Ctx.errorImagePixels[idx + 1] = 0x00;
                    s_Ctx.errorImagePixels[idx + 2] = 0x00;
                    s_Ctx.errorImagePixels[idx + 3] = 0xff;
                }
            }
        }
    }

    
}

void mfShutdown(void) {
    if(!s_Ctx.init) {
        printf("[MeltedForge]: MeltedForge's core is not yet initialized!");
        abort();
    }

    // Serializing
    {
        MFSerializer serializer = {0};
        mfSerializerCreate(&serializer, sizeof(u32) + sizeof(u64), false);
        
        mfSerializeU32(&serializer, MF_SIGNATURE_CORE_CACHE_FILE);
        mfSerializeU64(&serializer, s_Ctx.counter);

        b8 success = mfWriteFile(mfGetLogger(), serializer.bufferSize, "mfcore_cache.bin", serializer.buffer, "wb");

        if(success) {
            mfSerializerDestroy(&serializer);
        }
    }

    MF_FREEMEM(s_Ctx.errorImagePixels);
    s_Ctx.errorImageWidth = 0;
    s_Ctx.errorImageHeight = 0;

    slogLoggerDestroy(&s_Ctx.logger);

    s_Ctx.init = false;
    glfwTerminate();
}

u64 mfGetNextID(void) {
    if(!s_Ctx.init) {
        printf("[MeltedForge]: MeltedForge's core is not yet initialized!");
        abort();
    }
    
    if(s_Ctx.counter == UINT32_MAX) {
        slogLogMsg(&s_Ctx.logger, SLOG_SEVERITY_FATAL, "[MeltedForge]: Can't generate more IDs!");
        abort();
    }

    return s_Ctx.counter++;
}

u8* mfGetErrorImagePixels(void) {
    if(!s_Ctx.init) {
        printf("[MeltedForge]: MeltedForge's core is not yet initialized!");
        abort();
    }
    
    return s_Ctx.errorImagePixels;
}

u32 mfGetErrorImageWidth(void) {
    return s_Ctx.errorImageWidth;
}

u32 mfGetErrorImageHeight(void) {
    if(!s_Ctx.init) {
        printf("[MeltedForge]: MeltedForge's core is not yet initialized!");
        abort();
    }

    return s_Ctx.errorImageHeight;
}

SLogger* mfGetLogger(void) {
    if(!s_Ctx.init) {
        printf("[MeltedForge]: MeltedForge's core is not yet initialized!");
        abort();
    }
    
    return &s_Ctx.logger;
}

#ifdef __cplusplus
}
#endif