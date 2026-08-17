#ifdef __cplusplus
extern "C" {
#endif

#include "mfrendergraph.h"

/* 
 * TODO: Plan for a pass object, attachment object, and figure out how to return each attachment's imgui set if required 
 *       Also handle how to return each attachment's resource set or its image handle in case the client needs to get 
 *       its pixel data etc
*/

struct MFRenderGraph_s {
    MFRenderGraphConfig config;
    bool init, began;
    // TODO: complete these
};

// TODO: Implement all these functions!
MFRenderGraph* mfRenderGraphCreate(MFRenderer* renderer, MFRenderGraphConfig* config) {

}

void mfRenderGraphDestroy(MFRenderGraph* renderGraph) {

}

void mfRenderGraphInvoke(MFRenderGraph* renderGraph) {

}

#ifdef __cplusplus
}
#endif