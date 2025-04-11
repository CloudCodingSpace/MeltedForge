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

/*  Mat2  */

MF_INLINE MFMat2 mfMat2Identity() {
    MFMat2 mat;
    MF_SETMEM(mat.data, 0, sizeof(f32) * 4);

    mat.data[0] = 1.0f;
    mat.data[3] = 1.0f;

    return mat;
}

MF_INLINE MFMat2 mfMat2Add(MFMat2 a, MFMat2 b) {
    MFMat2 result;

    for(int i = 0; i < 4; i++) {
        result.data[i] = a.data[i] + b.data[i];
    }

    return result;
}

MF_INLINE MFMat2 mfMat2Sub(MFMat2 a, MFMat2 b) {
    MFMat2 result;

    for(int i = 0; i < 4; i++) {
        result.data[i] = a.data[i] - b.data[i];
    }

    return result;
}

MF_INLINE MFMat2 mfMat2Mul(MFMat2 a, MFMat2 b) {
    MFMat2 result;

    result.data[0] = a.data[0] * b.data[0] + a.data[2] * b.data[1];
    result.data[1] = a.data[1] * b.data[0] + a.data[3] * b.data[1];

    result.data[2] = a.data[0] * b.data[2] + a.data[2] * b.data[3];
    result.data[3] = a.data[1] * b.data[2] + a.data[3] * b.data[3];

    return result;
}

MF_INLINE MFMat2 mfMat2MulScalar(MFMat2 a, float x) {
    MFMat2 result;

    for (int i = 0; i < 4; i++) {
        result.data[i] = a.data[i] * x;
    } 

    return result;
}

MF_INLINE MFVec2 mfMat2MulVec2(MFMat2 a, MFVec2 x) {
    MFVec2 result;

    result.x = a.data[0] * x.x + a.data[1] * x.y;
    result.y = a.data[2] * x.x + a.data[3] * x.y;

    return result;
}

MF_INLINE void mfMat2Rotate(MFMat2* mat, float theta_rad) {
    float c = cosf(theta_rad);
    float s = sinf(theta_rad);
    
    MFMat2 rotation = {
        .data = {
            c, -s,
            s,  c
        }
    };

    *mat = mfMat2Mul(rotation, *mat);
}

MF_INLINE void mfMat2Scale(MFMat2* mat, float x, float y) {
    mat->data[0] = x;
    mat->data[3] = y;
}

MF_INLINE MFMat2 mfMat2Transpose(MFMat2 mat) {
    MFMat2 result;

    result.data[0] = mat.data[0];
    result.data[3] = mat.data[3];

    result.data[1] = mat.data[2];
    result.data[2] = mat.data[1];

    return result;
}

MF_INLINE MFMat2 mfMat2Inverse(MFMat2 mat) {
    MFMat2 result;

    result.data[0] = mat.data[3];
    result.data[1] = -mat.data[1];
    result.data[2] = -mat.data[2];
    result.data[3] = mat.data[0];

    float d = mat.data[0] * mat.data[3] - mat.data[1] * mat.data[2];

    if(d == 0) {
        MF_FATAL_ABORT(mfGetLogger(), "The inverse determinant is undefined! Aborting!");
    }

    d = 1.0f / d;

    result = mfMat2MulScalar(result, d);

    return result;
}

/*  Mat3  */
MF_INLINE MFMat3 mfMat3Identity() {
    MFMat3 mat;
    MF_SETMEM(mat.data, 0, sizeof(f32) * 9);

    mat.data[0] = 1.0f;
    mat.data[4] = 1.0f;
    mat.data[8] = 1.0f;

    return mat;
}

MF_INLINE MFMat3 mfMat3Add(MFMat3 a, MFMat3 b) {
    MFMat3 result;

    for(int i = 0; i < 9; i++) {
        result.data[i] = a.data[i] + b.data[i];
    }

    return result;
}

MF_INLINE MFMat3 mfMat3Sub(MFMat3 a, MFMat3 b) {
    MFMat3 result;

    for(int i = 0; i < 9; i++) {
        result.data[i] = a.data[i] - b.data[i];
    }

    return result;
}

MF_INLINE MFMat3 mfMat3Mul(MFMat3 a, MFMat3 b) {
    MFMat3 result;

    result.data[0] = a.data[0] * b.data[0] + a.data[3] * b.data[1] + a.data[6] * b.data[2];
    result.data[3] = a.data[0] * b.data[3] + a.data[3] * b.data[4] + a.data[6] * b.data[5];
    result.data[6] = a.data[0] * b.data[6] + a.data[3] * b.data[7] + a.data[6] * b.data[8];

    result.data[1] = a.data[1] * b.data[0] + a.data[4] * b.data[1] + a.data[7] * b.data[2];
    result.data[4] = a.data[1] * b.data[3] + a.data[4] * b.data[4] + a.data[7] * b.data[5];
    result.data[7] = a.data[1] * b.data[6] + a.data[4] * b.data[7] + a.data[7] * b.data[8];
    
    result.data[2] = a.data[2] * b.data[0] + a.data[5] * b.data[1] + a.data[8] * b.data[2];
    result.data[5] = a.data[2] * b.data[3] + a.data[5] * b.data[4] + a.data[8] * b.data[5];
    result.data[8] = a.data[2] * b.data[6] + a.data[5] * b.data[7] + a.data[8] * b.data[8];

    return result;
}

MF_INLINE MFMat3 mfMat3MulScalar(MFMat3 a, float x) {
    MFMat3 result;

    for (int i = 0; i < 9; i++) {
        result.data[i] = a.data[i] * x;
    } 

    return result;
}

MF_INLINE MFVec3 mfMat3MulVec3(MFMat3 a, MFVec3 x) {
    MFVec3 result;

    result.x = a.data[0] * x.x + a.data[1] * x.y + a.data[2] * x.z;
    result.y = a.data[3] * x.x + a.data[4] * x.y + a.data[5] * x.z;
    result.z = a.data[6] * x.x + a.data[7] * x.y + a.data[8] * x.z;

    return result;
}

MF_INLINE void mfMat3Rotate(MFMat3* mat, float theta_rad) {
    float c = cosf(theta_rad);
    float s = sinf(theta_rad);

    MFMat3 rotation = {
        .data = {
            c, -s, 0,
            s,  c, 0,
            0,  0, 1
        }
    };

    *mat = mfMat3Mul(rotation, *mat);
}

MF_INLINE void mfMat3Scale(MFMat3* mat, float x, float y, float z) {
    mat->data[0] = x;
    mat->data[4] = y;
    mat->data[8] = z;
}

MF_INLINE MFMat3 mfMat3Transpose(MFMat3 mat) {
    MFMat3 result;

    result.data[0] = mat.data[0];
    result.data[4] = mat.data[4];
    result.data[8] = mat.data[8];

    result.data[1] = mat.data[3];
    result.data[2] = mat.data[6];
    result.data[3] = mat.data[1];

    result.data[5] = mat.data[7];
    result.data[6] = mat.data[2];
    result.data[7] = mat.data[5];

    return result;
}

MF_INLINE void mfMat3Translate(MFMat3* mat, float tx, float ty) {
    MFMat3 translation = {
        .data = {
            1, 0, tx,
            0, 1, ty,
            0, 0, 1
        }
    };

    *mat = mfMat3Mul(translation, *mat);
}

MF_INLINE MFMat3 mfMat3Inverse(MFMat3 m) {
    float a = m.data[0], b = m.data[1], tx = m.data[2];
    float c = m.data[3], d = m.data[4], ty = m.data[5];

    float det = a * d - b * c;
    if (fabsf(det) < 1e-6f) {
        return (MFMat3){ .data = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        }};
    }

    float invDet = 1.0f / det;

    float ia =  d * invDet;
    float ib = -b * invDet;
    float ic = -c * invDet;
    float id =  a * invDet;

    float itx = -(ia * tx + ib * ty);
    float ity = -(ic * tx + id * ty);

    MFMat3 inv = {
        .data = {
            ia, ib, itx,
            ic, id, ity,
            0,  0,  1
        }
    };

    return inv;
}