#include "core.h"

#include <stdlib.h>
#include <string.h>

static MFState* s_mfState;

void mfInitialize() {
  s_mfState = (MFState*) malloc(sizeof(MFState));
  memset((void*)s_mfState, 0, sizeof(MFState));

  slogLoggerReset(&s_mfState->logger);
  slogLoggerSetName(&s_mfState->logger, "MFLogger");
}

void mfShutdown() {
  slogLoggerReset(&s_mfState->logger);
  free(s_mfState);
}

MFRenderer* mfGetRendererHandle() {
  return &s_mfState->renderer;
}

SLogger* mfGetSLoggerHandle() {
  return &s_mfState->logger;
}
