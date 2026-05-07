#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mfutils.h"

typedef struct MFArray_s {
    u64 len;
    u64 capacity;
    u64 elementSize;
    void* data;
    bool init;
} MFArray;

// @brief Creates a MFArray
// @return Returns a valid MFArray handle
// @param capacity A u64 which is the initial capacity of the array
// @param elementSize A u64 which is the size of each element of the array in bytes
MFArray mfArrayCreate(u64 capacity, u64 elementSize);

// @brief Destroys a MFArray
// @param array A valid pointer to MFArray returned by `mfArrayCreate`
void mfArrayDestroy(MFArray* array);

// @brief Resizes the array
// @param array A valid pointer to MFArray returned by `mfArrayCreate`
// @param newCapacity A u64 which is the new capacity of the array
void mfArrayResize(MFArray* array, u64 newCapacity);

// @brief Inserts an element at the given index
// @param array A valid pointer to MFArray returned by `mfArrayCreate`
// @param index A u64 which is the index where the elements needs to be inserted in the array
// @param element A void* which is the element to be inserted. Note the element given is copied into the index in the array, so the array doesn't have the memory ownership of the provided element handle
void mfArrayInsertAt(MFArray* array, u64 index, void* element);

// @brief Deletes an element in the given index
// @param array A valid pointer to MFArray returned by `mfArrayCreate`
// @param index A u64 which is the index where the elements needs to be deleted in the array
void mfArrayDeleteAt(MFArray* array, u64 index);

// @brief Gets the element in the array at the given index
// @param arr A valid MFArray handle returned by `mfArrayCreate`
// @param type Datatype of the element
// @param index The index of the element
#define mfArrayGetElement(arr, type, index) (((type*)(arr).data)[(index)])

// @brief Sets the element in the array at the given index
// @param arr A valid MFArray handle returned by `mfArrayCreate`
// @param type Datatype of the element
// @param index The index of the element
// @param element The element which needs to be set
#define mfArraySetElement(arr, type, index, element) (mfArrayGetElement(arr, type, index) = element)

// @brief Adds an element to the array
// @param arr A valid pointer to MFArray returned by `mfArrayCreate`
// @param type Datatype of the element
// @param element The element that needs to be added
#define mfArrayAddElement(arr, type, element) \
    do { \
        if ((arr)->len == (arr)->capacity) { \
            u64 newCap = (arr)->capacity == 0 ? 1 : (arr)->capacity * 2; \
            mfArrayResize((arr), newCap); \
        } \
        type tmp = (element); \
        memcpy(&mfArrayGetElement(*(arr), type, (arr)->len), &tmp, sizeof(type)); \
        (arr)->len++; \
    } while(0)

#ifdef __cplusplus
}
#endif