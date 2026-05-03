#ifdef __cplusplus
extern "C" {
#endif

#include "mfhashmap.h"
#include "mfcore.h"

MFHashMap mfHashMapCreate(u64 capacity, u64 valueSize) {
    MF_PANIC_IF(capacity == 0, mfGetLogger(), "The capacity of the hashmap provided can't be 0!");
    MF_PANIC_IF(valueSize == 0, mfGetLogger(), "The value size of the hashmap provided can't be 0!");

    MFHashMap hashmap = {
        .init = true,
        .buckets = mfArrayCreate(mfGetLogger(), capacity, sizeof(MFArray)),
        .valueSize = valueSize
    };

    hashmap.buckets.len = capacity;
    for(u64 i = 0; i < hashmap.buckets.len; i++) {
        mfArraySetElement(hashmap.buckets, MFArray, i, mfArrayCreate(mfGetLogger(), 1, sizeof(MFHashMapEntry)));
    }

    return hashmap;
}

void mfHashMapDestroy(MFHashMap* hashMap) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");

    for(u64 i = 0; i < hashMap->buckets.len; i++) {
        MFArray* bucket = &mfArrayGetElement(hashMap->buckets, MFArray, i);
        for(u64 j = 0; j < bucket->len; j++) {
            MFHashMapEntry* entry = &mfArrayGetElement(*bucket, MFHashMapEntry, j);
            MF_FREEMEM(entry->key);
            MF_FREEMEM(entry->value);
        }
        mfArrayDestroy(bucket, mfGetLogger());
    }

    mfArrayDestroy(&hashMap->buckets, mfGetLogger());

    MF_SETMEM(hashMap, 0, sizeof(MFHashMap));
}

void mfHashMapAddElement(MFHashMap* hashMap, u64 keySize, void* key, void* value) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");
    MF_PANIC_IF(keySize == 0, mfGetLogger(), "The key size of the hashmap provided can't be 0!");
    MF_PANIC_IF(key == mfnull, mfGetLogger(), "The key provided for the hashmap shouldn't be null!");
    MF_PANIC_IF(value == mfnull, mfGetLogger(), "The value provided for the hashmap shouldn't be null!");

    u64 hash = mfHash_FNV1A(key, keySize, mfGetLogger());
    u64 index = hash % hashMap->buckets.capacity;

    MFArray* bucket = &mfArrayGetElement(hashMap->buckets, MFArray, index);

    for (u64 i = 0; i < bucket->len; i++) {
        MFHashMapEntry* entry = &mfArrayGetElement(*bucket, MFHashMapEntry, i);

        if (entry->hash == hash && entry->keySize == keySize && (memcmp(entry->key, key, keySize) == 0)) {
            memcpy(entry->value, value, hashMap->valueSize);
            return;
        }
    }

    MFHashMapEntry newEntry = {
        .hash = hash,
        .key = MF_ALLOCMEM(void, keySize),
        .value = MF_ALLOCMEM(void, hashMap->valueSize),
        .keySize = keySize
    };

    memcpy(newEntry.key, key, keySize);
    memcpy(newEntry.value, value, hashMap->valueSize);

    mfArrayAddElement(bucket, MFHashMapEntry, mfGetLogger(), newEntry);
}

void* mfHashMapGetValue(MFHashMap* hashMap, u64 keySize, void* key) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");
    MF_PANIC_IF(keySize == 0, mfGetLogger(), "The key size of the hashmap provided can't be 0!");
    MF_PANIC_IF(key == mfnull, mfGetLogger(), "The key provided for the hashmap shouldn't be null!");

    u64 hash = mfHash_FNV1A(key, keySize, mfGetLogger());
    u64 index = hash % hashMap->buckets.capacity;

    MFArray* bucket = &mfArrayGetElement(hashMap->buckets, MFArray, index);
    for(u64 i = 0; i < bucket->len; i++) {
        MFHashMapEntry* entry = &mfArrayGetElement(*bucket, MFHashMapEntry, i);
        if(entry->hash == hash && entry->keySize == keySize && (memcmp(entry->key, key, keySize) == 0))
            return entry->value;
    }

    return mfnull;
}

u64 mfHash_FNV1A(const void* data, u64 size, SLogger* logger) {
    MF_PANIC_IF(data == mfnull, logger, "The data provided for hashing shouldn't be null!");
    if(size == 0) {
        return 0;
    }

    const u8 *p = (const u8*)data;

    u64 hash = 1469598103934665603ULL;

    for (u64 i = 0; i < size; i++) {
        hash ^= p[i];
        hash *= 1099511628211ULL;
    }

    return hash;
}

#ifdef __cplusplus
}
#endif
