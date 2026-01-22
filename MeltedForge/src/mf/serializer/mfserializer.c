#include "mfserializer.h"

void mfSerializerCreate(MFSerializer* serializer, u64 bufferSize) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(bufferSize == 0, mfGetLogger(), "The serializer's bufferSize provided shouldn't be 0!");
    
    serializer->bufferSize = bufferSize + (sizeof(u32) * 2); //* NOTE: FOR THE 2 SIGNATURES
    serializer->offset = 0;
    serializer->buffer = MF_ALLOCMEM(u8, serializer->bufferSize);

    //Serializing the header & version
    {
        u32 header = MF_SIGNATURE_HEADER;
        u32 version = MF_SIGNATURE_VERSION;

        memcpy(serializer->buffer + serializer->offset, &header, sizeof(u32));
        serializer->offset += sizeof(u32);
        memcpy(serializer->buffer + serializer->offset, &version, sizeof(u32));
        serializer->offset += sizeof(u32);
    }
}

void mfSerializerDestroy(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    
    MF_FREEMEM(serializer->buffer);
    MF_SETMEM(serializer, 0, sizeof(MFSerializer));
}

void mfSerializerRewind(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");

    serializer->offset = sizeof(u32) * 2; //* For skipping past the signatures while deserializing!
}

b8 mfSerializerIfValid(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_DO_IF(serializer->bufferSize < (sizeof(u32)*2), {
        return false;
    });

    u64 offset = serializer->offset;
    serializer->offset = 0;
    u32 header = 0;
    memcpy(&header, serializer->buffer + serializer->offset, sizeof(u32));
    b8 b = header == MF_SIGNATURE_HEADER;
    serializer->offset = offset;

    return b;
}

b8 mfSerializerIfSameVersion(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_DO_IF(serializer->bufferSize < (sizeof(u32) * 2), {
        return false;
    });

    u64 offset = serializer->offset;
    serializer->offset = sizeof(u32); //* NOTE: For the header signature
    u32 ver = 0;
    memcpy(&ver, serializer->buffer + serializer->offset, sizeof(u32));
    b8 b = ver == MF_SIGNATURE_VERSION;
    serializer->offset = offset;
    return b;
}

void mfSerializeI8(MFSerializer* serializer, i8 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(i8)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(i8));
    serializer->offset += sizeof(i8);
}

void mfSerializeU8(MFSerializer* serializer, u8 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(u8)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
    
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(u8));
    serializer->offset += sizeof(u8);
}

void mfSerializeI16(MFSerializer* serializer, i16 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(i16)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    MF_DO_IF((mfSerializerIfValid(serializer) && mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(i16));
    serializer->offset += sizeof(i16);
}

void mfSerializeU16(MFSerializer* serializer, u16 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(u16)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(u16));
    serializer->offset += sizeof(u16);
}

void mfSerializeI32(MFSerializer* serializer, i32 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(i32)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(i32));
    serializer->offset += sizeof(i32);
}

void mfSerializeU32(MFSerializer* serializer, u32 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(u32)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(u32));
    serializer->offset += sizeof(u32);
}

void mfSerializeI64(MFSerializer* serializer, i64 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(i64)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(i64));
    serializer->offset += sizeof(i64);
}

void mfSerializeU64(MFSerializer* serializer, u64 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(u64)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });

    memcpy(serializer->buffer + serializer->offset, &value, sizeof(u64));
    serializer->offset += sizeof(u64);
}

void mfSerializeF32(MFSerializer* serializer, f32 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(f32)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
 
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });
   
    memcpy(serializer->buffer + serializer->offset, &value, sizeof(f32));
    serializer->offset += sizeof(f32);
}

void mfSerializeF64(MFSerializer* serializer, f64 value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(f64)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
 
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });
  
    memcpy(serializer->buffer + serializer->offset, &value, sizeof(f64));
    serializer->offset += sizeof(f64);
}

void mfSerializeB8(MFSerializer* serializer, b8 value) {
    mfSerializeU8(serializer, (u8)value);
}

void mfSerializeChar(MFSerializer* serializer, char value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(char)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });
    
    memcpy(serializer->buffer + serializer->offset, &value, sizeof(char));
    serializer->offset += sizeof(char);
}

void mfSerializeString(MFSerializer* serializer, const char* value) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't serialize data since the output buffer isn't supported by the current serializing api! (Maybe the version is different, or any other issue!)\n");
        return;
    });   

    //! NOTE: Things may break if the length is too long for a u32 to store, which is very less likely but still to be noted!
    u64 stringSize = (sizeof(char) * mfStringLen(mfGetLogger(), value)) + sizeof(u32);
    MF_PANIC_IF((serializer->offset + stringSize) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
    
    mfSerializeU32(serializer, mfStringLen(mfGetLogger(), value));
    for(u32 i = 0; i < mfStringLen(mfGetLogger(), value); i++) {
        mfSerializeChar(serializer, value[i]);
    }
}

i8 mfDeserializeI8(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(i8)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
    
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the output buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return 0;
    });

    i8 value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(i8));
    serializer->offset += sizeof(i8);
    return value;
}

u8 mfDeserializeU8(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(u8)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
     
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return 0;
    });
   
    u8 value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(u8));
    serializer->offset += sizeof(u8);
    return value;    
}

i16 mfDeserializeI16(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(i16)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
   
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return 0;
    });
    
    i16 value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(i16));
    serializer->offset += sizeof(i16);
    return value;
}

u16 mfDeserializeU16(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(u16)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return 0;
    });

    u16 value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(u16));
    serializer->offset += sizeof(u16);
    return value;            
}

i32 mfDeserializeI32(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(i32)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
 
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return 0;
    });

    i32 value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(i32));
    serializer->offset += sizeof(i32);
    return value;            
}

u32 mfDeserializeU32(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(u32)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
   
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return 0;
    }); 

    u32 value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(u32));
    serializer->offset += sizeof(u32);
    return value;
}

i64 mfDeserializeI64(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(i64)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
   
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return 0;
    });

    i64 value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(i64));
    serializer->offset += sizeof(i64);
    return value;
}

u64 mfDeserializeU64(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(u64)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
   
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return 0;
    });

    u64 value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(u64));
    serializer->offset += sizeof(u64);
    return value;
}

f32 mfDeserializeF32(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(f32)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
   
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return 0;
    });

    f32 value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(f32));
    serializer->offset += sizeof(f32);
    return value;
}

f64 mfDeserializeF64(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(f64)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
   
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return 0;
    });

    f64 value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(f64));
    serializer->offset += sizeof(f64);
    return value;
}

b8 mfDeserializeB8(MFSerializer* serializer) {
    return (b8)mfDeserializeU8(serializer);
}

char mfDeserializeChar(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(char)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
   
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return '\0';
    });

    char value = 0;
    memcpy(&value, serializer->buffer + serializer->offset, sizeof(char));
    serializer->offset += sizeof(char);
    return value;
}

//! NOTE: Things may break if the length is too long for a u32 to store, which is very less likely but still to be noted!
char* mfDeserializeString(MFSerializer* serializer) {
    MF_PANIC_IF(serializer == mfnull, mfGetLogger(), "The serializer handle provided shouldn't be null!");
    MF_PANIC_IF(serializer->buffer == mfnull, mfGetLogger(), "The serializer handle provided should be created!");
    MF_PANIC_IF((serializer->offset + sizeof(u32)) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");
   
    MF_DO_IF((!mfSerializerIfValid(serializer)) && (!mfSerializerIfSameVersion(serializer)), {
        slogLogConsole(mfGetLogger(), SLOG_SEVERITY_ERROR, "Can't deserialize data since the input buffer isn't supported by the current deserializing api! (Maybe the version is different, or any other issue!)\n");
        return "\0";
    });

    u32 len = mfDeserializeU32(serializer);
    u64 stringSize = sizeof(char) * len;
    MF_PANIC_IF((serializer->offset + stringSize) > serializer->bufferSize, mfGetLogger(), "Serializer buffer out of memory!");

    u64 size = sizeof(char) * (len + 1); // +1 for the '\0'
    char* string = MF_ALLOCMEM(char, size);

    for(u32 i = 0; i < len; i++) {
        string[i] = mfDeserializeChar(serializer);
    }

    string[len] = '\0';
    return string;
}
