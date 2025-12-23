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