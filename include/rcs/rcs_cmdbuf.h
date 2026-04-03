#ifndef RCS_FB_H
#define RCS_FB_H
#include "backend/backend.h"

bool make_rcs_cmdbuf(RenderBackend* rb, VkCommandPool cpool,
                     VkCommandBuffer* out_cmdbuf, CleanupStack* cs);

#endif