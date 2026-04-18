#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"

#include "mfserializerutils.h"

typedef struct MFSerializer_s {
    u64 bufferSize;
    u8* buffer;
    u64 offset;
} MFSerializer;

void mfSerializerCreate(MFSerializer* serializer, u64 bufferSize, bool deserializer);
void mfSerializerDestroy(MFSerializer* serializer);
void mfSerializerRewind(MFSerializer* serializer);

bool mfSerializerIfValid(MFSerializer* serializer);
bool mfSerializerIfSameVersion(MFSerializer* serializer);

void mfSerializeI8(MFSerializer* serializer, i8 value);
void mfSerializeU8(MFSerializer* serializer, u8 value);
void mfSerializeI16(MFSerializer* serializer, i16 value);
void mfSerializeU16(MFSerializer* serializer, u16 value);
void mfSerializeI32(MFSerializer* serializer, i32 value);
void mfSerializeU32(MFSerializer* serializer, u32 value);
void mfSerializeI64(MFSerializer* serializer, i64 value);
void mfSerializeU64(MFSerializer* serializer, u64 value);

void mfSerializeF32(MFSerializer* serializer, f32 value);
void mfSerializeF64(MFSerializer* serializer, f64 value);

void mfSerializeB8(MFSerializer* serializer, bool value);

void mfSerializeChar(MFSerializer* serializer, char value);
void mfSerializeString(MFSerializer* serializer, const char* value);

i8 mfDeserializeI8(MFSerializer* serializer);
u8 mfDeserializeU8(MFSerializer* serializer);
i16 mfDeserializeI16(MFSerializer* serializer);
u16 mfDeserializeU16(MFSerializer* serializer);
i32 mfDeserializeI32(MFSerializer* serializer);
u32 mfDeserializeU32(MFSerializer* serializer);
i64 mfDeserializeI64(MFSerializer* serializer);
u64 mfDeserializeU64(MFSerializer* serializer);

f32 mfDeserializeF32(MFSerializer* serializer);
f64 mfDeserializeF64(MFSerializer* serializer);

bool mfDeserializeB8(MFSerializer* serializer);

char mfDeserializeChar(MFSerializer* serializer);
// @note The returned char* must be freed since it is allocated on the heap
char* mfDeserializeString(MFSerializer* serializer);

#ifdef __cplusplus
}
#endif