#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mfutils.h"
#include "mfarray.h"

typedef struct MFHashMap_s {
    MFArray buckets;
    u64 valueSize, keySize, count;
    bool init;
} MFHashMap;

MFHashMap mfHashMapCreate(u64 capacity, u64 keySize, u64 valueSize);
void mfHashMapDestroy(MFHashMap* hashMap);

void mfHashMapResize(MFHashMap* hashMap, u64 newCapacity);
void mfHashMapAddElement(MFHashMap* hashMap, void* key, void* value);
void mfHashMapRemoveElement(MFHashMap* hashMap, void* key);
void* mfHashMapGetValue(MFHashMap* hashMap, void* key);

// @note A little modified version of FNV-1a-64
u64 mfHash_FNV1A(const void* data, u64 size);

#ifdef __cplusplus
}
#endif