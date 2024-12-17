#include "core.h"
#include "mf/renderer.h"

#include <stdlib.h>
#include <string.h>

static MFState* s_mfState;

void mfInitialize(MFBckndTypes type) {
  s_mfState = (MFState*) malloc(sizeof(MFState));
  memset((void*)s_mfState, 0, sizeof(MFState));

  slogLoggerReset(&s_mfState->logger);
  slogLoggerSetName(&s_mfState->logger, "MFLogger");

  mfRendererInit(&s_mfState->renderer, type);
}

void mfShutdown() {
  mfRendererDeinit(&s_mfState->renderer);
  slogLoggerReset(&s_mfState->logger);
  free(s_mfState);
}

MFRenderer* mfGetRendererHandle() {
  return &s_mfState->renderer;
}

SLogger* mfGetSLoggerHandle() {
  return &s_mfState->logger;
}
