#ifndef COMMANDPOOL_H
#define COMMANDPOOL_H
#include "backend/backend.h"
#include "cleanupstack.h"
#include "common.h"

bool make_commandpool(VkDevice dev, Queues queues, VkCommandPool* pool, struct Error* e_out,
                      CleanupStack* cs);

bool make_commandbuffers(VkDevice dev, VkCommandPool pool, u32 n_max_inflight,
                         VkCommandBuffer** cmdbufs, struct Error* e_out, CleanupStack* cs);

#endif