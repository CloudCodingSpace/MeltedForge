#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <slog/slog.h>

#define mfnull 0

#ifndef true
    #define true 1
#endif
#ifndef false
    #define false 0
#endif

#define MF_ALLOCMEM(T, size) ((T*)memset((T*)malloc(size), 0, (size)))
#define MF_SETMEM(mem, val, size) do { memset((mem), (val), (size)); } while(0)
#define MF_FREEMEM(mem) do { if((mem)) { free((void*)(mem)); (mem) = 0; } } while(0)

#define MF_MIN(x, y) ((x) < (y) ? (x) : (y))
#define MF_MAX(x, y) ((x) > (y) ? (x) : (y))
#define MF_CLAMP(value, min, max) (((value) <= (min)) ? (min) : ((value) >= (max)) ? (max) : (value))

#define MF_FATAL_ABORT(logger, msg) do { slogLogConsole((logger), SLOG_SEVERITY_FATAL, (msg)); abort(); } while(0)
#define MF_ASSERT(expr, logger, msg) do { if ((expr)) { MF_FATAL_ABORT((logger), (msg)); } } while(0)

#ifdef _DEBUG
    #define MF_INFO(logger, msg) do { slogLogConsole((logger), SLOG_SEVERITY_INFO, (msg)); } while(0)
#else
    #define MF_INFO(logger, msg) do {} while(0)
#endif

#if defined(__clang__) || defined(__gcc__)
    #define MF_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define MF_INLINE __forceinline
#else
    #define MF_INLINE static inline
#endif

#define MF_ARRAYLEN(arr, T) (sizeof(arr) / sizeof(T))

typedef float f32;
typedef double f64;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef uint8_t b8;

// @note The returned const char* must be freed since it is allocated on the heap
MF_INLINE char* mfReadFile(SLogger* logger, u64* size, const char* path, const char* mode) {
    MF_ASSERT(path == 0, logger, "The file path provided shouldn't be null!");
    MF_ASSERT(mode == 0, logger, "The file reading mode provided shouldn't be null!");
    MF_ASSERT(size == 0, logger, "The size pointer provided shouldn't be null!");

    char* content;
    FILE* file = fopen(path, mode);
    MF_ASSERT(file == 0, logger, "Failed to open the file! Most probably because the file doesn't exist or the reading mode is wrong!");

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    content = MF_ALLOCMEM(char, sizeof(char) * (*size) + 1);
    fread(content, sizeof(char), *size, file);
    fclose(file);

    content[*size] = '\0';
    return content;
}

MF_INLINE void mfWriteFile(SLogger* logger, u64 size, const char* path, const char* data, const char* mode) {
    MF_ASSERT(path == 0, logger, "The file path provided shouldn't be null!");
    MF_ASSERT(mode == 0, logger, "The file reading mode provided shouldn't be null!");
    MF_ASSERT(data == 0, logger, "The data pointer provided shouldn't be null!");
    MF_ASSERT(size == 0, logger, "The size provided shouldn't be 0!");

    FILE* file = fopen(path, mode);
    MF_ASSERT(file == 0, logger, "Failed to open the file! Most probably because the file doesn't exist or the reading mode is wrong!");

    fwrite(data, 1, size, file);

    fclose(file);
}

MF_INLINE u32 mfStringLen(SLogger* logger, const char* a) {
    MF_ASSERT(a == mfnull, logger, "The string provided to find the length shouldn't be null!");

    i32 i = 0;
    while (true) {
        if (a[i] == '\0')
            return i;
        i++;
    }
}

// @note The returned const char* must be freed since it is allocated on the heap
MF_INLINE const char* mfStringConcatenate(SLogger* logger, const char* a, const char* b) {
    MF_ASSERT(a == mfnull, logger, "The first string provided shouldn't be null!");
    MF_ASSERT(b == mfnull, logger, "The second string provided shouldn't be null!");

    i32 lena = mfStringLen(logger, a);
    i32 len = lena + mfStringLen(logger, b) + 1;
    char* final = MF_ALLOCMEM(char, sizeof(char) * len);

    for (i32 i = 0; i < lena; i++) {
        final[i] = a[i];
    }

    for (i32 i = lena; i < len; i++) {
        i32 j = i - lena;
        final[i] = b[j];
    }

    final[len] = '\0';
    return final;
}

// @note The returned const char* must be freed since it is allocated on the heap
MF_INLINE const char* mfStringDuplicate(SLogger* logger, const char* a) {
    MF_ASSERT(a == mfnull, logger, "The string provided shouldn't be null!");

    i32 len = mfStringLen(logger, a);
    char* str = MF_ALLOCMEM(char, sizeof(char) * len);
    memcpy(str, a, sizeof(char) * len);
    str[len] = '\0';

    return str;
}

MF_INLINE i32 mfStringFind(SLogger* logger, const char* s, const char a) {
    MF_ASSERT(s == mfnull, logger, "The string provided shouldn't be null!");

    i32 i = 0;
    while (true) {
        if (s[i] == '\0')
            return -1;
        if (s[i] == a)
            return i;
        i++;
    }
}

MF_INLINE i32 mfStringFindLast(SLogger* logger, const char* s, const char a) {
    MF_ASSERT(s == mfnull, logger, "The string provided shouldn't be null!");
    
    i32 i = 0;
    i32 idx = -1;
    while (true) {
        if (s[i] == '\0')
        return idx;
        if (s[i] == a)
        idx = i;
        i++;
    }
}

MF_INLINE b8 mfStringEndsWith(SLogger* logger, const char* a, const char* b) {
    MF_ASSERT(a == mfnull, logger, "The string provided shouldn't be null!");
    MF_ASSERT(b == mfnull, logger, "The string provided shouldn't be null!");

    if(mfStringLen(logger, a) < mfStringLen(logger, b))
        return false;

    for(int i = 0; i < strlen(b); i++) {
        int j = mfStringLen(logger, a) - mfStringLen(logger, b) + i;
        if(a[j] != b[i])
            return false;
    }

    return true;
}

typedef struct MFArray_s {
    u64 len;
    u64 capacity;
    u64 elementSize;
    void* data;
} MFArray;

MF_INLINE MFArray mfArrayCreate(SLogger* logger, u64 len, u64 elementSize) {
    MF_ASSERT(len == 0, logger, "The length of the new array can't be allocated and set as 0!");
    MF_ASSERT(elementSize == 0, logger, "The element size of the new array can't be allocated and set as 0!");

    MFArray array = {0};
    array.elementSize = elementSize;
    array.capacity = len;

    array.data = malloc(elementSize * len);
    memset(array.data, 0, elementSize * len);

    return array;
}

MF_INLINE void mfArrayDestroy(MFArray* array, SLogger* logger) {
    MF_ASSERT(array == mfnull, logger, "The array provided shouldn't be 0!");

    if (array->capacity == 0)
        return;

    free(array->data);
    memset(array, 0, sizeof(MFArray));
}

MF_INLINE void mfArrayResize(MFArray* array, u64 newCapacity, SLogger* logger) {
    MF_ASSERT(array == mfnull, logger, "The array provided shouldn't be 0!");

    if (newCapacity == 0) {
        MF_FATAL_ABORT(logger, "New capacity cannot be zero!");
    }

    if (newCapacity <= array->capacity)
        return;

    void* newData = realloc(array->data, array->elementSize * newCapacity);
    if (!newData) {
        MF_FATAL_ABORT(logger, "Failed to allocate memory for array resize!");
    }

    memset((u8*)newData + (array->elementSize * array->capacity), 0, (newCapacity - array->capacity) * array->elementSize);

    array->data = newData;
    array->capacity = newCapacity;
}

#define mfArrayGet(arr, type, index) (((type*)(arr).data)[(index)])
#define mfArrayAddElement(arr, type, logger, element) \
    do { \
        if ((arr).len == (arr).capacity) { \
            u64 newCap = (arr).capacity == 0 ? 1 : (arr).capacity * 2; \
            mfArrayResize(&(arr), newCap, (logger)); \
        } \
        type tmp = (element); \
        memcpy(&mfArrayGet((arr), type, (arr).len), &tmp, sizeof(type)); \
        (arr).len++; \
    } while(0)
