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

void mfSerializeF32(MFSerializer* serializer, f32 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(f32)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
    
    memcpy(serializer->buffer + serializer->offset, &value, sizeof(f32));
    serializer->offset += sizeof(f32);
}

void mfSerializeF64(MFSerializer* serializer, f64 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(f64)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
    
    memcpy(serializer->buffer + serializer->offset, &value, sizeof(f64));
    serializer->offset += sizeof(f64);
}

void mfSerializeB8(MFSerializer* serializer, b8 value) {
    mfSerializeU8(serializer, (u8)value);
}

void mfSerializeChar(MFSerializer* serializer, char value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF((serializer->offset + sizeof(char)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
    
    memcpy(serializer->buffer + serializer->offset, &value, sizeof(char));
    serializer->offset += sizeof(char);
}

void mfSerializeString(MFSerializer* serializer, char* value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    
    //! NOTE: Things may break if the length is too long for a u32 to store, which is very less likely but still to be noted!
    u64 stringSize = (sizeof(char) * mfStringLen(mfGetLogger(), value)) + sizeof(u32);
    MF_PANIC_IF((serializer->offset + stringSize) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
    
    mfSerializeU32(serializer, mfStringLen(mfGetLogger(), value));
    for(u32 i = 0; i < mfStringLen(mfGetLogger(), value); i++) {
        mfSerializeChar(serializer, value[i]);
    }
}