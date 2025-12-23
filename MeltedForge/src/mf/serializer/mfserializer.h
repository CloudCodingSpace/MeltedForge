#pragma once

#include "core/mfcore.h"
#include "core/mfutils.h"

typedef struct MFSerializer_s {
    u64 bufferSize;
    u8* buffer;
    u64 offset;
} MFSerializer;

void mfSerializerCreate(MFSerializer* serializer, u64 bufferSize);
void mfSerializerDestroy(MFSerializer* serializer);