#ifdef __cplusplus
extern "C" {
#endif

#include "mfhashmap.h"
#include "mfcore.h"

MFHashMap mfHashMapCreate(u64 capacity, u64 keySize, u64 valueSize) {
    MF_PANIC_IF(capacity == 0, mfGetLogger(), "The capacity of the hashmap provided can't be 0!");
    MF_PANIC_IF(keySize == 0, mfGetLogger(), "The key size of the hashmap provided can't be 0!");
    MF_PANIC_IF(valueSize == 0, mfGetLogger(), "The value size of the hashmap provided can't be 0!");

    MFHashMap hashmap = {
        .init = true,
        .buckets = mfArrayCreate(capacity, sizeof(MFArray)),
        .valueSize = valueSize,
        .keySize = keySize
    };

    hashmap.buckets.len = capacity;
    for(u64 i = 0; i < hashmap.buckets.len; i++) {
        mfArraySetElement(hashmap.buckets, MFArray, i, mfArrayCreate(1, sizeof(u64) + keySize + valueSize));
    }

    return hashmap;
}

void mfHashMapDestroy(MFHashMap* hashMap) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");

    for(u64 i = 0; i < hashMap->buckets.len; i++) {
        MFArray* bucket = &mfArrayGetElement(hashMap->buckets, MFArray, i);
        mfArrayDestroy(bucket);
    }

    mfArrayDestroy(&hashMap->buckets);

    MF_SETMEM(hashMap, 0, sizeof(MFHashMap));
}

void mfHashMapResize(MFHashMap* hashMap, u64 newCapacity) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");
    MF_PANIC_IF(newCapacity == 0, mfGetLogger(), "The new capacity of the hashmap provided can't be 0!");

    if(newCapacity == hashMap->buckets.capacity)
        return;

    MFArray newBuckets = mfArrayCreate(newCapacity, sizeof(MFArray));
    newBuckets.len = newCapacity;

    for (u64 i = 0; i < newCapacity; i++) {
        mfArraySetElement(newBuckets, MFArray, i, mfArrayCreate(1, sizeof(u64) + hashMap->keySize + hashMap->valueSize));
    }

    for (u64 i = 0; i < hashMap->buckets.len; i++) {
        MFArray* oldBucket = &mfArrayGetElement(hashMap->buckets, MFArray, i);

        for (u64 j = 0; j < oldBucket->len; j++) {
            u8* entry = &mfArrayGetElement(*oldBucket, u8, j * oldBucket->elementSize);
            u64* entry_hash = (u64*)entry;
            void* entry_key = entry + sizeof(u64);
            void* entry_value = ((u8*)entry_key) + hashMap->keySize;

            u64 newIndex = *entry_hash % newCapacity;
            MFArray* newBucket = &mfArrayGetElement(newBuckets, MFArray, newIndex);

            if (newBucket->len == newBucket->capacity) {
                mfArrayResize(newBucket, newBucket->capacity * 2);
            }

            memcpy((u8*)newBucket->data + newBucket->len * newBucket->elementSize,
                entry,
                newBucket->elementSize);

            newBucket->len++;
        }

        mfArrayDestroy(oldBucket);
    }

    mfArrayDestroy(&hashMap->buckets);
    hashMap->buckets = newBuckets;
}

void mfHashMapAddElement(MFHashMap* hashMap, void* key, void* value) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");
    MF_PANIC_IF(key == mfnull, mfGetLogger(), "The key provided for the hashmap shouldn't be null!");
    MF_PANIC_IF(value == mfnull, mfGetLogger(), "The value provided for the hashmap shouldn't be null!");

    if((hashMap->count * 4) >= (hashMap->buckets.len * 3)) {
        mfHashMapResize(hashMap, hashMap->buckets.len * 2);
    }

    u64 hash = mfHash_FNV1A(key, hashMap->keySize);
    u64 index = hash % hashMap->buckets.len;

    MFArray* bucket = &mfArrayGetElement(hashMap->buckets, MFArray, index);

    for (u64 i = 0; i < bucket->len; i++) {
        u8* entry = &mfArrayGetElement(*bucket, u8, i * bucket->elementSize);
        u64* entry_hash = (u64*)entry;
        void* entry_key = entry + sizeof(u64);
        void* entry_value = ((u8*)entry_key) + hashMap->keySize;

        if(*entry_hash != hash)
            continue;
        if (memcmp(entry_key, key, hashMap->keySize) == 0) {
            memcpy(entry_value, value, hashMap->valueSize);
            return;
        }
    }

    u8* newEntry = MF_ALLOCMEM(u8, bucket->elementSize);
    memcpy(newEntry, &hash, sizeof(u64));
    memcpy(newEntry + sizeof(u64), key, hashMap->keySize);
    memcpy(newEntry + sizeof(u64) + hashMap->keySize, value, hashMap->valueSize);

    if (bucket->len == bucket->capacity) {
        mfArrayResize(bucket, bucket->capacity * 2);
    }

    memcpy(&mfArrayGetElement(*bucket, u8, bucket->len * bucket->elementSize), newEntry, bucket->elementSize);
    bucket->len++;

    hashMap->count++;
    MF_FREEMEM(newEntry);
}

void mfHashMapRemoveElement(MFHashMap* hashMap, void* key) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");
    MF_PANIC_IF(key == mfnull, mfGetLogger(), "The key provided for the hashmap shouldn't be null!");
    
    u64 hash = mfHash_FNV1A(key, hashMap->keySize);
    u64 index = hash % hashMap->buckets.len;

    MFArray* bucket = &mfArrayGetElement(hashMap->buckets, MFArray, index);
    for(u64 i = 0; i < bucket->len; i++) {
        u8* entry = &mfArrayGetElement(*bucket, u8, i * bucket->elementSize);
        u64* entry_hash = (u64*)entry;
        void* entry_key = entry + sizeof(u64);
        void* entry_value = ((u8*)entry_key) + hashMap->keySize;

        if(*entry_hash != hash)
            continue;
        if(memcmp(entry_key, key, hashMap->keySize) == 0) {
            if(bucket->len > 1) {
                u8* last = &mfArrayGetElement(*bucket, u8, (bucket->len - 1) * bucket->elementSize);
                memcpy(entry, last, bucket->elementSize);
            }
            hashMap->count--;
            bucket->len--;
            return;
        }
    }
}

void* mfHashMapGetValue(MFHashMap* hashMap, void* key) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");
    MF_PANIC_IF(key == mfnull, mfGetLogger(), "The key provided for the hashmap shouldn't be null!");

    u64 hash = mfHash_FNV1A(key, hashMap->keySize);
    u64 index = hash % hashMap->buckets.len;

    MFArray* bucket = &mfArrayGetElement(hashMap->buckets, MFArray, index);
    for(u64 i = 0; i < bucket->len; i++) {
        u8* entry = &mfArrayGetElement(*bucket, u8, i * bucket->elementSize);
        u64* entry_hash = (u64*)entry;
        void* entry_key = entry + sizeof(u64);
        void* entry_value = ((u8*)entry_key) + hashMap->keySize;

        if(*entry_hash != hash)
            continue;
        if(memcmp(entry_key, key, hashMap->keySize) == 0)
            return entry_value;
    }

    return mfnull;
}

u64 mfHash_FNV1A(const void* data, u64 size) {
    MF_PANIC_IF(data == mfnull, mfGetLogger(), "The data provided for hashing shouldn't be null!");
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
