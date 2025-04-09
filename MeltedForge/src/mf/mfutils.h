#pragma once

#include <stdlib.h>
#include <string.h>

#define MF_ALLOCMEM(T, size) (T*)memset((T*)malloc(size), 0, size)
#define MF_SETMEM(mem, val, size) memset(mem, val, size)
#define MF_FREEMEM(mem) free(mem)

#define MFMIN(x, y) (x < y ? x : y)
#define MFMAX(x, y) (x > y ? x : y)

#define MFCLAMP(value, min, max) ((value <= min) ? min : (value >= max) ? max : value)

#if defined(__clang__) || defined(__gcc__)
    #define MF_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define MF_INLINE __forceinline
#else
    #define MF_INLINE static inline
#endif

#define MF_ARRAYLEN(arr, T) (sizeof(arr)/sizeof(T)) //! The arr must be statically allocated