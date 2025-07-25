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

#include "mf/core/mfcore.h"
#include "mf/core/mfutils.h"
#include "mf/core/mfmaths.h"
#include "mf/core/mfentry.h"
#include "mf/core/mftimer.h"

// UI
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>