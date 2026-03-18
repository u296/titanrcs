#ifndef COMMANDPOOL_H
#define COMMANDPOOL_H
#include "cleanupstack.h"
#include "common.h"
#include "device.h"



bool make_commandpool(VkDevice dev, struct Queues queues, VkCommandPool* pool, struct Error* e_out, CleanupStack*cs);



bool make_commandbuffers(VkDevice dev, VkCommandPool pool, u32 n_max_inflight, VkCommandBuffer** cmdbufs, struct Error* e_out, CleanupStack*cs);

bool recordcommandbuffer(VkExtent2D swapchainextent, VkFramebuffer fb, VkCommandBuffer cmdbuf, VkRenderPass renderpass, VkDescriptorSet desc_set, Renderable ren, struct Error* e_out);


#endif