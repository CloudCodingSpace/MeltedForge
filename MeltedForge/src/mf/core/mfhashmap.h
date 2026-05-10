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

// @brief Creates a hashmap
// @return Returns a valid MFHashMap handle
// @param capacity A u64 which specifies the total capacity of the hash map
// @param keySize A u64 which specifies the size of each key in bytes of the hashmap
// @param valueSize A u64 which specifies the size of each value in bytes of the hashmap 
MFHashMap mfHashMapCreate(u64 capacity, u64 keySize, u64 valueSize);

// @brief Destroys a valid MFHashMap handle
// @param hashMap A valid MFHashMap* returned by `mfHashMapCreate`
void mfHashMapDestroy(MFHashMap* hashMap);

// @bried Resizes a hashmap
// @param hashMap A valid MFHashMap* returned by `mfHashMapCreate`
// @param newCapacity A u64 which specifies the new capacity of the hashmap
void mfHashMapResize(MFHashMap* hashMap, u64 newCapacity);

// @brief Adds a element to the end of a hashmap if the key isn't found, else just updates it's value
// @param hashMap A valid MFHashMap* returned by `mfHashMapCreate`
// @param key A pointer to the key
// @param value A pointer to the value
void mfHashMapAddElement(MFHashMap* hashMap, void* key, void* value);

// @brief Removes an element in the hashmap
// @param hashMap A valid MFHashMap* returned by `mfHashMapCreate`
// @param key A pointer to the key of the element which needs to be removed
void mfHashMapRemoveElement(MFHashMap* hashMap, void* key);

// @brief Gets a value of a given key from the hashmap. If key isn't present, then returns `mfnull` aka `0`
// @param hashMap A valid MFHashMap* returned by `mfHashMapCreate`
// @param key A pointer to the key
void* mfHashMapGetValue(MFHashMap* hashMap, void* key);

// @brief A common, and fast 64-bit hashing algorithm called `FNV1A`
// @note A little modified version of FNV-1a-64 and can't really be used for security purposes since this hash isn't that strong
// @return Returns the hash
// @param data A pointer to the data which needs to be hashed
// @param size The size of the data in bytes
u64 mfHash_FNV1A(const void* data, u64 size);

#ifdef __cplusplus
}
#endif