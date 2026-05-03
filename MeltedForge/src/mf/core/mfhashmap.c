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
        .buckets = mfArrayCreate(capacity, sizeof(MFArray)),
        .valueSize = valueSize
    };

    hashmap.buckets.len = capacity;
    for(u64 i = 0; i < hashmap.buckets.len; i++) {
        mfArraySetElement(hashmap.buckets, MFArray, i, mfArrayCreate(1, sizeof(MFHashMapEntry)));
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
        mfArraySetElement(newBuckets, MFArray, i, mfArrayCreate(1, sizeof(MFHashMapEntry)));
    }

    for (u64 i = 0; i < hashMap->buckets.len; i++) {
        MFArray* oldBucket = &mfArrayGetElement(hashMap->buckets, MFArray, i);

        for (u64 j = 0; j < oldBucket->len; j++) {
            MFHashMapEntry* entry = &mfArrayGetElement(*oldBucket, MFHashMapEntry, j);

            u64 newIndex = entry->hash % newCapacity;
            MFArray* newBucket = &mfArrayGetElement(newBuckets, MFArray, newIndex);

            mfArrayAddElement(newBucket, MFHashMapEntry, *entry);
        }

        MF_FREEMEM(oldBucket->data);
    }

    MF_FREEMEM(hashMap->buckets.data);
    hashMap->buckets = newBuckets;
}

void mfHashMapAddElement(MFHashMap* hashMap, u64 keySize, void* key, void* value) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");
    MF_PANIC_IF(keySize == 0, mfGetLogger(), "The key size of the hashmap provided can't be 0!");
    MF_PANIC_IF(key == mfnull, mfGetLogger(), "The key provided for the hashmap shouldn't be null!");
    MF_PANIC_IF(value == mfnull, mfGetLogger(), "The value provided for the hashmap shouldn't be null!");

    if((hashMap->count * 4) >= (hashMap->buckets.len * 3)) {
        mfHashMapResize(hashMap, hashMap->buckets.len * 2);
    }

    u64 hash = mfHash_FNV1A(key, keySize);
    u64 index = hash % hashMap->buckets.len;

    MFArray* bucket = &mfArrayGetElement(hashMap->buckets, MFArray, index);

    for (u64 i = 0; i < bucket->len; i++) {
        MFHashMapEntry* entry = &mfArrayGetElement(*bucket, MFHashMapEntry, i);

        if(entry->hash != hash)
            continue;
        if(entry->keySize != keySize)
            continue;
        if (memcmp(entry->key, key, keySize) == 0) {
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

    mfArrayAddElement(bucket, MFHashMapEntry, newEntry);
    hashMap->count++;
}

void mfHashMapRemoveElement(MFHashMap* hashMap, u64 keySize, void* key) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");
    MF_PANIC_IF(keySize == 0, mfGetLogger(), "The key size of the hashmap provided can't be 0!");
    MF_PANIC_IF(key == mfnull, mfGetLogger(), "The key provided for the hashmap shouldn't be null!");
    
    u64 hash = mfHash_FNV1A(key, keySize);
    u64 index = hash % hashMap->buckets.len;

    MFArray* bucket = &mfArrayGetElement(hashMap->buckets, MFArray, index);
    for(u64 i = 0; i < bucket->len; i++) {
        MFHashMapEntry* entry = &mfArrayGetElement(*bucket, MFHashMapEntry, i);

        if(entry->hash != hash)
            continue;
        if(entry->keySize != keySize)
            continue;
        if(memcmp(entry->key, key, keySize) == 0) {
            MF_FREEMEM(entry->key);
            MF_FREEMEM(entry->value);
            if(bucket->len > 1) {
                MFHashMapEntry* last = &mfArrayGetElement(*bucket, MFHashMapEntry, bucket->len - 1);
                *entry = *last;
            }
            hashMap->count--;
            bucket->len--;
            return;
        }
    }
}

void* mfHashMapGetValue(MFHashMap* hashMap, u64 keySize, void* key) {
    MF_PANIC_IF(hashMap == mfnull, mfGetLogger(), "The hash map handle provided shouldn't be null!");
    MF_PANIC_IF(!hashMap->init, mfGetLogger(), "The hash map provided isn't intialised!");
    MF_PANIC_IF(keySize == 0, mfGetLogger(), "The key size of the hashmap provided can't be 0!");
    MF_PANIC_IF(key == mfnull, mfGetLogger(), "The key provided for the hashmap shouldn't be null!");

    u64 hash = mfHash_FNV1A(key, keySize);
    u64 index = hash % hashMap->buckets.len;

    MFArray* bucket = &mfArrayGetElement(hashMap->buckets, MFArray, index);
    for(u64 i = 0; i < bucket->len; i++) {
        MFHashMapEntry* entry = &mfArrayGetElement(*bucket, MFHashMapEntry, i);
        if(entry->hash != hash)
            continue;
        if(entry->keySize != keySize)
            continue;
        if(memcmp(entry->key, key, keySize) == 0)
            return entry->value;
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
