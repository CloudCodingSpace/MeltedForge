// @note: Only to be included by other projects

#include "mf/window/mfwindow.h"
#include "mf/window/mfinput.h"

#include "mf/renderer/mfrenderer.h"
#include "mf/renderer/mfpipeline.h"
#include "mf/renderer/mfbuffer.h"
#include "mf/renderer/mfimage.h"
#include "mf/renderer/mfcamera.h"
#include "mf/renderer/mfmesh.h"
#include "mf/renderer/mfmodel.h"

#include "mf/ecs/mfcomponents.h"
#include "mf/ecs/mfentity.h"
#include "mf/ecs/mfscene.h"

#include "mf/core/mfcore.h"
#include "mf/core/mfapp.h"
#include "mf/core/mfutils.h"
#include "mf/core/mfmaths.h"
#include "mf/core/mftimer.h"

#ifdef MF_INCLUDE_ENTRY
    #include "mf/core/mfentry.h"
#endif

#include "mf/systems/material_system.h"

#include "mf/serializer/mfserializerutils.h"
#include "mf/serializer/mfserializer.h"

// UI
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>