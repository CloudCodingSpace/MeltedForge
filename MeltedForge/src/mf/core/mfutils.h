#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <slog/slog.h>

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

#define MF_ARRAYLEN(arr, T) (sizeof(arr)/sizeof(T)) //! The arr must be statically allocated

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