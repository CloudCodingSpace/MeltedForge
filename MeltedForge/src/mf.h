// @note: Only to be included by other projects

#include "mf/window/mfwindow.h"
#include "mf/window/mfinput.h"

#include "mf/renderer/mfrenderer.h"
#include "mf/renderer/mfpipeline.h"
#include "mf/renderer/mfgpubuffer.h"
#include "mf/renderer/mfgpuimage.h"
#include "mf/renderer/mfgpu_res.h"

#include "mf/objects/mfcamera.h"
#include "mf/objects/mfmesh.h"
#include "mf/objects/mfmodel.h"

#include "mf/ecs/mfcomponents.h"
#include "mf/ecs/mfentity.h"
#include "mf/ecs/mfscene.h"

#include "mf/core/mfcore.h"
#include "mf/core/mfapp.h"
#include "mf/core/mfprofiler.h"
#include "mf/core/mfutils.h"
#include "mf/core/mfmaths.h"
#include "mf/core/mftime.h"

#ifdef MF_INCLUDE_ENTRY
    #include "mf/core/mfentry.h"
#endif

#include "mf/systems/mfmaterial_system.h"

#include "mf/serializer/mfserializerutils.h"
#include "mf/serializer/mfserializer.h"

// Other
#include <cimgui.h>
#include <slog/slog.h>