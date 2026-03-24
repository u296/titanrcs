#ifndef SYNC_H
#define SYNC_H
#include "cleanupstack.h"
#include "common.h"
#include "vulkan/vulkan_core.h"

bool make_sync_objects(VkDevice dev, const u32 n_max_inflight, const u32 n_swpch_img, VkSemaphore** sem1,
                       VkSemaphore** sem2, VkFence** fen, struct Error* e_out, CleanupStack* cs);

#endif