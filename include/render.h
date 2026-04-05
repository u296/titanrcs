#ifndef RENDER_H
#define RENDER_H

#include "context.h"

typedef enum LoopStatus { REMAKE_SWAPCHAIN, EXIT_PROGRAM } LoopStatus;

LoopStatus renderloop_visualonly(RenderContext* ctx);

#endif