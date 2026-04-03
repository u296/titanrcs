#ifndef RCS_RENDER_H
#define RCS_RENDER_H

#include "context.h"

void render_rcs_imgs(RenderContext* ctx, u32 f);

void record_rcs_cmdbuf(RenderContext* ctx, u32 f);

void write_rcs_ubo(RenderContext* ctx, void* mapping);

#endif