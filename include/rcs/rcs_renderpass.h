#ifndef RCS_RENDERPASS_H
#define RCS_RENDERPASS_H

#include "common.h"
#include "backend/backend.h"

bool make_rcs_renderpass(RenderBackend* rb, VkRenderPass* renderpass, struct Error* e_out,
                         CleanupStack* cs);

#endif