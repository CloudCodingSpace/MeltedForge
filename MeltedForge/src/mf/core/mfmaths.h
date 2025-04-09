#pragma once

#include "mfutils.h"

#define MF_PI 3.14159265358979323846f
#define MF_2PI (2.0f * MF_PI)
#define MF_4PI (4.0f * MF_PI)
#define MF_HALF_PI (0.5f * MF_PI)
#define MF_QUARTER_PI (0.25f * MF_PI)
#define MF_ONE_OVER_PI (1.0f / MF_PI)
#define MF_ONE_OVER_TWO_PI (1.0f / MF_2PI)

#define MF_SQRT_TWO 1.41421356237309504880f
#define MF_SQRT_THREE 1.73205080756887729352f
#define MF_SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define MF_SQRT_ONE_OVER_THREE 0.57735026918962576450f

#define MF_DEG2RAD_MULTIPLIER (MF_PI / 180.0f)
#define MF_RAD2DEG_MULTIPLIER (180.0f / MF_PI)

#define MF_SEC_TO_US_MULTIPLIER (1000.0f * 1000.0f)
#define MF_SEC_TO_MS_MULTIPLIER 1000.0f
#define MF_MS_TO_SEC_MULTIPLIER 0.001f

#define MF_INFINITY (1e30f * 1e30f)
#define MF_FLOAT_EPSILON 1.192092896e-07f
#define MF_FLOAT_MIN -3.40282e+38F
#define MF_FLOAT_MAX 3.40282e+38F

typedef struct MFVec2_s {
    union {
        f32 x, r, s, u;
    };

    union {
        f32 y, g, t, v;
    };
} MFVec2;

typedef struct MFVec3_s {
    union {
        f32 x, r, s, u;
    };

    union {
        f32 y, g, t, v;
    };

    union {
        f32 z, b, p, w;
    };
} MFVec3;

typedef struct MFVec4_s {
    union {
        f32 x, r, s;
    };

    union {
        f32 y, g, t;
    };

    union {
        f32 z, b, width;
    };

    union {
        f32 w, a, height;
    };
} MFVec4;

typedef struct MFMat2_s {
    f32 data[4];
} MFMat2;

typedef struct MFMat3_s {
    f32 data[9];
} MFMat3;

typedef struct MFMat4_s {
    f32 data[16];
} MFMat4;