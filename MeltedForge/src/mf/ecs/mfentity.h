#pragma once

#include "core/mfutils.h"

typedef struct MFEntity_s {
    void* ownerScene;
    u64 uuid;
    u32 id;
    u32 components;
} MFEntity;