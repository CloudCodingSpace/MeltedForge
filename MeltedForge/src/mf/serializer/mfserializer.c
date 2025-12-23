#include "mfserializer.h"

void mfSerializerCreate(MFSerializer* serializer, u64 bufferSize) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(bufferSize == 0, mfGetLogger(), "The serializer's bufferSize provided shouldn't be 0!");
    
    serializer->bufferSize = bufferSize;
    serializer->offset = 0;
    serializer->buffer = MF_ALLOCMEM(u8, bufferSize);
}

void mfSerializerDestroy(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    
    MF_FREEMEM(serializer->buffer);
    MF_SETMEM(serializer, 0, sizeof(MFSerializer));
}

void mfSerializeI8(MFSerializer* serializer, i8 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(i8)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(i8));
    serializer->offset += sizeof(i8);
}

void mfSerializeU8(MFSerializer* serializer, u8 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(u8)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(u8));
    serializer->offset += sizeof(u8);
}

void mfSerializeI16(MFSerializer* serializer, i16 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(i16)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(i16));
    serializer->offset += sizeof(i16);
}

void mfSerializeU16(MFSerializer* serializer, u16 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(u16)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(u16));
    serializer->offset += sizeof(u16);
}

void mfSerializeI32(MFSerializer* serializer, i32 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(i32)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(i32));
    serializer->offset += sizeof(i32);
}

void mfSerializeU32(MFSerializer* serializer, u32 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(u32)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(u32));
    serializer->offset += sizeof(u32);
}

void mfSerializeI64(MFSerializer* serializer, i64 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(i64)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(i64));
    serializer->offset += sizeof(i64);
}

void mfSerializeU64(MFSerializer* serializer, u64 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(u64)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(u64));
    serializer->offset += sizeof(u64);
}