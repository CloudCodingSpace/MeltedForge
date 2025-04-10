#pragma once

#include "mfutils.h"

#include <stdalign.h>
#include <math.h>

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
    alignas(16) f32 data[4];
} MFMat2;

typedef struct MFMat3_s {
    alignas(16) f32 data[9];
} MFMat3;

typedef struct MFMat4_s {
    alignas(16) f32 data[16];
} MFMat4;

/*     Vec2      */

MF_INLINE MFVec2 mfVec2Create(f32 x, f32 y) {
    return (MFVec2){x, y};
}

MF_INLINE MFVec2 mfVec2Add(MFVec2 a, MFVec2 b) {
    return (MFVec2){a.x + b.x, a.y + b.y};
}

MF_INLINE MFVec2 mfVec2Sub(MFVec2 a, MFVec2 b) {
    return (MFVec2){a.x - b.x, a.y - b.y};
}

MF_INLINE MFVec2 mfVec2Mul(MFVec2 a, MFVec2 b) {
    return (MFVec2){a.x * b.x, a.y * b.y};
}

MF_INLINE MFVec2 mfVec2MulScalar(MFVec2 a, float x) {
    return (MFVec2){a.x * x, a.y * x};
}

MF_INLINE MFVec2 mfVec2Div(MFVec2 a, MFVec2 b) {
    return (MFVec2){a.x / b.x, a.y / b.y};
}

MF_INLINE float mfVec2LengthSqr(MFVec2 vec) {
    return vec.x * vec.x + vec.y * vec.y;
}

MF_INLINE float mfVec2Length(MFVec2 vec) {
    return sqrtf(mfVec2LengthSqr(vec));
}

MF_INLINE float mfVec2Dot(MFVec2 a, MFVec2 b) {
    return a.x * b.x + a.y * b.y;
}

MF_INLINE MFVec2 mfVec2Normalize(MFVec2 a) {
    f32 len = mfVec2Length(a);
    if(len == 0.0f)
        return mfVec2Create(0, 0);
    return mfVec2Div(a, mfVec2Create(len, len));
}

/*     Vec3      */

MF_INLINE MFVec3 mfVec3Create(f32 x, f32 y, f32 z) {
    return (MFVec3){x, y, z};
}

MF_INLINE MFVec3 mfVec3Add(MFVec3 a, MFVec3 b) {
    return (MFVec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

MF_INLINE MFVec3 mfVec3Sub(MFVec3 a, MFVec3 b) {
    return (MFVec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

MF_INLINE MFVec3 mfVec3Mul(MFVec3 a, MFVec3 b) {
    return (MFVec3){a.x * b.x, a.y * b.y, a.z * b.z};
}

MF_INLINE MFVec3 mfVec3MulScalar(MFVec3 a, float x) {
    return (MFVec3){a.x * x, a.y * x, a.z * x};
}

MF_INLINE MFVec3 mfVec3Div(MFVec3 a, MFVec3 b) {
    return (MFVec3){a.x / b.x, a.y / b.y, a.z / b.z};
}

MF_INLINE float mfVec3LengthSqr(MFVec3 vec) {
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}

MF_INLINE float mfVec3Length(MFVec3 vec) {
    return sqrtf(mfVec3LengthSqr(vec));
}

MF_INLINE float mfVec3Dot(MFVec3 a, MFVec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

MF_INLINE MFVec3 mfVec3Cross(MFVec3 a, MFVec3 b) {
    return (MFVec3) {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

MF_INLINE MFVec3 mfVec3Normalize(MFVec3 a) {
    f32 len = mfVec3Length(a);
    if(len == 0.0f)
        return mfVec3Create(0, 0, 0);
    return mfVec3Div(a, mfVec3Create(len, len, len));
}

MF_INLINE MFVec3 mfVec3Reflect(MFVec3 I, MFVec3 N) {
    return mfVec3Sub(I, mfVec3MulScalar(N, 2.0f * mfVec3Dot(I, N)));
}

/*     Vec4      */

MF_INLINE MFVec4 mfVec4Create(f32 x, f32 y, f32 z, f32 w) {
    return (MFVec4){x, y, z, w};
}

MF_INLINE MFVec4 mfVec4Add(MFVec4 a, MFVec4 b) {
    return (MFVec4){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

MF_INLINE MFVec4 mfVec4Sub(MFVec4 a, MFVec4 b) {
    return (MFVec4){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

MF_INLINE MFVec4 mfVec4Mul(MFVec4 a, MFVec4 b) {
    return (MFVec4){a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

MF_INLINE MFVec4 mfVec4MulScalar(MFVec4 a, float x) {
    return (MFVec4){a.x * x, a.y * x, a.z * x, a.w * x};
}

MF_INLINE MFVec4 mfVec4Div(MFVec4 a, MFVec4 b) {
    return (MFVec4){a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
}

MF_INLINE float mfVec4LengthSqr(MFVec4 vec) {
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w;
}

MF_INLINE float mfVec4Length(MFVec4 vec) {
    return sqrtf(mfVec4LengthSqr(vec));
}

MF_INLINE float mfVec4Dot(MFVec4 a, MFVec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

MF_INLINE MFVec4 mfVec4Normalize(MFVec4 a) {
    f32 len = mfVec4Length(a);
    if(len == 0.0f)
        return mfVec4Create(0, 0, 0, 0);
    return mfVec4Div(a, mfVec4Create(len, len, len, len));
}