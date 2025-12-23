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

void mfSerializeI8(MFSerializer* serializer, i8 value);
void mfSerializeU8(MFSerializer* serializer, u8 value);
void mfSerializeI16(MFSerializer* serializer, i16 value);
void mfSerializeU16(MFSerializer* serializer, u16 value);
void mfSerializeI32(MFSerializer* serializer, i32 value);
void mfSerializeU32(MFSerializer* serializer, u32 value);
void mfSerializeI64(MFSerializer* serializer, i64 value);
void mfSerializeU64(MFSerializer* serializer, u64 value);