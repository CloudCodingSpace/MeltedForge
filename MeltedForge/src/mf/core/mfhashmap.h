#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mfutils.h"
#include "mfarray.h"

typedef struct MFHashMapEntry_s {
    u64 hash;
    void* key;
    void* value;
    u64 keySize;
} MFHashMapEntry;

typedef struct MFHashMap_s {
    MFArray buckets;
    u64 valueSize, count;
    bool init;
} MFHashMap;

MFHashMap mfHashMapCreate(u64 capacity, u64 valueSize);
void mfHashMapDestroy(MFHashMap* hashMap);

void mfHashMapResize(MFHashMap* hashMap, u64 newCapacity);
void mfHashMapAddElement(MFHashMap* hashMap, u64 keySize, void* key, void* value);
void mfHashMapRemoveElement(MFHashMap* hashMap, u64 keySize, void* key);
void* mfHashMapGetValue(MFHashMap* hashMap, u64 keySize, void* key);

// @note A little modified version of FNV-1a-64
u64 mfHash_FNV1A(const void* data, u64 size);

#ifdef __cplusplus
}
#endif