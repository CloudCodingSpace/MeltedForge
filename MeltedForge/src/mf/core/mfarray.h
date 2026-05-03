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

MFArray mfArrayCreate(u64 capacity, u64 elementSize);
void mfArrayDestroy(MFArray* array);
void mfArrayResize(MFArray* array, u64 newCapacity);
void mfArrayInsertAt(MFArray* array, u64 index, void* element);
void mfArrayDeleteAt(MFArray* array, u64 index);

#define mfArrayGetElement(arr, type, index) (((type*)(arr).data)[(index)])
#define mfArraySetElement(arr, type, index, element) (mfArrayGetElement(arr, type, index) = element)
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