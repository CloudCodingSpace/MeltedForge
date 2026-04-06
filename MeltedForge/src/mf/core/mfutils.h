#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <slog/slog.h>

#pragma region defines

#define mfnull 0

#ifndef true
    #define true 1
#endif
#ifndef false
    #define false 0
#endif

#define MF_ALLOCMEM(T, size) ((T*)calloc(1, size))
#define MF_SETMEM(mem, val, size) do { memset((mem), (val), (size)); } while(0)
#define MF_FREEMEM(mem) do { if((mem)) { free((void*)(mem)); (mem) = 0; } } while(0)

#define MF_MIN(x, y) ((x) < (y) ? (x) : (y))
#define MF_MAX(x, y) ((x) > (y) ? (x) : (y))
#define MF_CLAMP(value, min, max) (((value) <= (min)) ? (min) : ((value) >= (max)) ? (max) : (value))

#if defined(MF_DEBUG) && !defined(MF_NDEBUG)
    #define MF_INFO(logger, msg, ...) do { slogLogMsg((logger), SLOG_SEVERITY_DEBUG, (msg), ##__VA_ARGS__); } while(0)
#else
    #define MF_INFO(logger, msg, ...) do {} while(0)
#endif

#define MF_FATAL_ABORT(logger, msg, ...) do { slogLogMsg((logger), SLOG_SEVERITY_FATAL, (msg), ##__VA_ARGS__); abort(); } while(0) 
#define MF_PANIC_IF(expr, logger, msg, ...) do { if ((expr)) { MF_FATAL_ABORT((logger), (msg), ##__VA_ARGS__); } } while(0)
#define MF_DO_IF(expr, work) do { if ((expr)) { {work}; } } while(0)

#if defined(__clang__) || defined(__gcc__)
    #define MF_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define MF_INLINE __forceinline
#else
    #define MF_INLINE static inline
#endif

//* @note Only works if arr is an array, and not if it is a pointer to the array!
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

#pragma endregion

#pragma region file_funcs

// @note The returned const char* must be freed since it is allocated on the heap
MF_INLINE char* mfReadFile(SLogger* logger, u64* size, b8* success, const char* path, const char* mode) {
    MF_PANIC_IF(path == 0, logger, "The file path provided shouldn't be null!");
    MF_PANIC_IF(mode == 0, logger, "The file reading mode provided shouldn't be null!");
    MF_PANIC_IF(size == 0, logger, "The size pointer provided shouldn't be null!");
    MF_PANIC_IF(success == 0, logger, "The success pointer provided shouldn't be null!");

    FILE* file = fopen(path, mode);
    if(file == mfnull) {
        *success = false;
        return mfnull;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = MF_ALLOCMEM(char, sizeof(char) * (*size) + 1);
    fread(buffer, sizeof(char), *size, file);
    fclose(file);

    buffer[*size] = '\0';

    *success = true;
    return buffer;
}

MF_INLINE b8 mfWriteFile(SLogger* logger, u64 size, const char* path, const char* data, const char* mode) {
    MF_PANIC_IF(path == 0, logger, "The file path provided shouldn't be null!");
    MF_PANIC_IF(mode == 0, logger, "The file reading mode provided shouldn't be null!");
    MF_PANIC_IF(data == 0, logger, "The data pointer provided shouldn't be null!");
    MF_PANIC_IF(size == 0, logger, "The size provided shouldn't be 0!");

    FILE* file = fopen(path, mode);
    if(file == mfnull) {
        return false;
    }

    fwrite(data, 1, size, file);

    fclose(file);
    return true;
}

#pragma endregion

#pragma region string_funcs

// @note The returned char* must be freed since it is allocated on the heap
#define mfStringDuplicate(a) strdup(a)
#define mfStringLen(a) strlen(a)
#define mfStringCompare(a) strcmp(a)

// @note The returned char* must be freed since it is allocated on the heap
MF_INLINE char* mfStringConcatenate(SLogger* logger, const char* a, const char* b) {
    MF_PANIC_IF(a == mfnull, logger, "The first string provided shouldn't be null!");
    MF_PANIC_IF(b == mfnull, logger, "The second string provided shouldn't be null!");

    i32 lena = mfStringLen(a);
    i32 len = lena + mfStringLen(b) + 1;
    char* final = MF_ALLOCMEM(char, sizeof(char) * len);

    for (i32 i = 0; i < lena; i++) {
        final[i] = a[i];
    }

    for (i32 i = lena; i < len; i++) {
        i32 j = i - lena;
        final[i] = b[j];
    }

    return final;
}

// @note The returned char* must be freed since it is allocated on the heap
MF_INLINE char* mfStringSliceLeft(SLogger* logger, const char* a, i32 idx) {
    MF_PANIC_IF(a == mfnull, logger, "The string provided shouldn't be null!");
    MF_PANIC_IF((idx < 0) || (idx >= mfStringLen(a)), logger, "The string index provided should be valid!");
    
    char* str = MF_ALLOCMEM(char, sizeof(char) * (mfStringLen(a) - idx));
    u64 j = 0;
    for(u32 i = idx; i < mfStringLen(a); i++) {
        str[j] = a[i];
        j++;
    }
    str[j++] = '\0';

    return str;
}

// @note The returned char* must be freed since it is allocated on the heap
MF_INLINE char* mfStringSliceRight(SLogger* logger, const char* a, i32 idx) {
    MF_PANIC_IF(a == mfnull, logger, "The string provided shouldn't be null!");
    MF_PANIC_IF((idx < 0) || (idx >= mfStringLen(a)), logger, "The string index provided should be valid!");
    
    char* str = MF_ALLOCMEM(char, (idx + 3) * sizeof(char));
    u64 i;
    for(i = 0; i <= idx; i++) {
        str[i] = a[i];
    }
    str[i++] = '/';
    str[i++] = '\0';

    return str;
}

MF_INLINE i32 mfStringFind(SLogger* logger, const char* s, const char a) {
    MF_PANIC_IF(s == mfnull, logger, "The string provided shouldn't be null!");

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
    MF_PANIC_IF(s == mfnull, logger, "The string provided shouldn't be null!");
    
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
    MF_PANIC_IF(a == mfnull, logger, "The string provided shouldn't be null!");
    MF_PANIC_IF(b == mfnull, logger, "The string provided shouldn't be null!");

    u32 lena = mfStringLen(a);
    u32 lenb = mfStringLen(b);
    if(lena < lenb)
        return false;

    for(u32 i = 0; i < lenb; i++) {
        u32 j = lena - lenb + i;
        if(a[j] != b[i])
            return false;
    }

    return true;
}

#pragma endregion

#pragma region mfarray

typedef struct MFArray_s {
    u64 len;
    u64 capacity;
    u64 elementSize;
    void* data;
    b8 init;
} MFArray;

MF_INLINE MFArray mfArrayCreate(SLogger* logger, u64 capacity, u64 elementSize) {
    MF_PANIC_IF(capacity == 0, logger, "The length of the new array can't be allocated and set as 0!");
    MF_PANIC_IF(elementSize == 0, logger, "The element size of the new array can't be allocated and set as 0!");

    MFArray array = {0};
    array.elementSize = elementSize;
    array.capacity = capacity;

    array.data = malloc(elementSize * capacity);
    memset(array.data, 0, elementSize * capacity);

    array.init = true;
    return array;
}

MF_INLINE void mfArrayDestroy(MFArray* array, SLogger* logger) {
    MF_PANIC_IF(array == mfnull, logger, "The array provided shouldn't be 0!");
    MF_PANIC_IF(!array->init, logger, "The array provided isn't initialised!");

    if (array->capacity == 0)
        return;

    MF_FREEMEM(array->data);
    MF_SETMEM(array, 0, sizeof(MFArray));
}

MF_INLINE void mfArrayResize(MFArray* array, u64 newCapacity, SLogger* logger) {
    MF_PANIC_IF(array == mfnull, logger, "The array provided shouldn't be 0!");
    MF_PANIC_IF(!array->init, logger, "The array provided isn't initialised!");
    
    if (newCapacity == 0) {
        MF_FATAL_ABORT(logger, "New capacity cannot be zero!");
    }
    
    if (newCapacity <= array->capacity)
        return;
        
    void* newData = realloc(array->data, array->elementSize * newCapacity);
    if (!newData) {
        MF_FATAL_ABORT(logger, "Failed to reallocate memory for array resize!");
    }
    
    MF_SETMEM((u8*)newData + (array->elementSize * array->capacity), 0, (newCapacity - array->capacity) * array->elementSize);
    
    array->data = newData;
    array->capacity = newCapacity;
}

MF_INLINE void mfArrayInsertAt(MFArray* array, u64 index, void* element, SLogger* logger) {
    MF_PANIC_IF(array == mfnull, logger, "The array provided shouldn't be 0!");
    MF_PANIC_IF(!array->init, logger, "The array provided isn't initialised!");
    MF_DO_IF(index >= array->len, {
        slogLogMsg(logger, SLOG_SEVERITY_WARN, "The index provided to MFArray is invalid!");
        return;
    });

    if(array->len == array->capacity) {
        mfArrayResize(array, (array->capacity == 0) ? 1 : (array->capacity * 2), logger);
    }

    u64 left = array->len - index;
    void* leftData = MF_ALLOCMEM(void, array->elementSize * left);
    u8* data = (u8*)array->data;
    memcpy(leftData, data + (array->elementSize * index), left * array->elementSize);
    MF_SETMEM(data + (array->elementSize * index), 0, left * array->elementSize);
    memcpy(data + (array->elementSize * index), element, array->elementSize);
    memcpy(data + (array->elementSize * (index + 1)), leftData, left * array->elementSize);

    MF_FREEMEM(leftData);
    array->len++;
}

MF_INLINE void mfArrayDeleteAt(MFArray* array, u64 index, SLogger* logger) {
    MF_PANIC_IF(array == mfnull, logger, "The array provided shouldn't be 0!");
    MF_PANIC_IF(!array->init, logger, "The array provided isn't initialised!");
    MF_DO_IF(index >= array->len, {
        slogLogMsg(logger, SLOG_SEVERITY_WARN, "The index provided to MFArray is invalid!");
        return;
    });

    u64 left = array->len - index + 1;
    void* leftData = MF_ALLOCMEM(void, array->elementSize * left);
    u8* data = (u8*)array->data;
    memcpy(leftData, data + (array->elementSize * (index + 1)), left * array->elementSize);
    MF_SETMEM(data + (array->elementSize * index), 0, array->elementSize);
    memcpy(data + (array->elementSize * index), leftData, left * array->elementSize);

    MF_FREEMEM(leftData);
    array->len--;
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


#pragma endregion
