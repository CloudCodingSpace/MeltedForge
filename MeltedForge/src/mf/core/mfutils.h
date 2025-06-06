#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <slog/slog.h>

#ifndef true
    #define true 1
#endif
#ifndef false
    #define false 0
#endif

#define MF_ALLOCMEM(T, size) (T*)memset((T*)malloc(size), 0, size)
#define MF_SETMEM(mem, val, size) memset(mem, val, size)
#define MF_FREEMEM(mem) free(mem); mem = 0

#define MF_MIN(x, y) (x < y ? x : y)
#define MF_MAX(x, y) (x > y ? x : y)

#define MF_FATAL_ABORT(logger, msg) slogLogConsole(logger, SLOG_SEVERITY_FATAL, msg); abort()
#define MF_ASSERT(expr, logger, msg) if(expr) { MF_FATAL_ABORT(logger, msg); }

#ifdef _DEBUG
    #define MF_INFO(logger, msg) slogLogConsole(logger, SLOG_SEVERITY_INFO, msg)
#else
    #define MF_INFO(logger, msg)
#endif

#define MF_CLAMP(value, min, max) ((value <= min) ? min : (value >= max) ? max : value)

#if defined(__clang__) || defined(__gcc__)
    #define MF_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define MF_INLINE __forceinline
#else
    #define MF_INLINE static inline
#endif

// @note The returned const char* must be freed since it is allocated on the heap
MF_INLINE char* mfReadFile(SLogger* logger, size_t* size, const char* path, const char* mode) {
    MF_ASSERT(path == 0, logger, "The file path provided shouldn't be null!");
    MF_ASSERT(mode == 0, logger, "The file reading mode provided shouldn't be null!");
    MF_ASSERT(size == 0, logger, "The size pointer provided shouldn't be null!");

    char* content;

    FILE* file = fopen(path, mode);
    MF_ASSERT(file == 0, logger, "Failed to open the file! Most probably because the file doesn't exists or the reading mode is wrong!");

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    content = MF_ALLOCMEM(char, sizeof(char) * (*size) + 1);
    fread(content, sizeof(char), *size, file);

    fclose(file);

    content[*size] = '\0';
    return content;
}

#define MF_ARRAYLEN(arr, T) (sizeof(arr)/sizeof(T)) //! The arr must be allocated in the stack

typedef float f32;
typedef double f64;
typedef signed int i32;
typedef unsigned int u32;
typedef signed long long i64;
typedef unsigned long long u64;
typedef char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef bool b8;