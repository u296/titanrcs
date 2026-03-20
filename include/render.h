#ifndef RENDER_H
#define RENDER_H

#include "context.h"

typedef enum LoopStatus { REMAKE_SWAPCHAIN, EXIT_PROGRAM } LoopStatus;

LoopStatus do_renderloop(RenderContext* ctx);

#endif