#pragma once

#include "core/mfutils.h"

#define MF_CREATE_SIGNATURE_BYTES(a, b, c, d) ((u32)(((u8)a << 24) | ((u8)b << 16) | ((u8)c << 8) | ((u8)d << 0)))

//* NOTE: May add more signatures in the future! Also, each signature is a u32 in type!
typedef enum MFSignatures_e {
    MF_SIGNATURE_HEADER = MF_CREATE_SIGNATURE_BYTES('M', 'F', 'H', 'R'),
    MF_SIGNATURE_VERSION = MF_CREATE_SIGNATURE_BYTES('V', 'D', 'E', 'V')
} MFSignatures;