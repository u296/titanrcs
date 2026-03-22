#ifndef RENDERRESOURCES_H
#define RENDERRESOURCES_H
#include "cleanupstack.h"
#include "common.h"

typedef struct RenderContext RenderContext;

typedef struct RenderResources {
    u32 n_inflight_frames;
    VkDescriptorPool dpool;
    VkCommandPool cmd_pool;
    VkCommandBuffer* cmd_bufs;
    VkSemaphore* img_ready_sems;
    VkSemaphore* render_finished_sems;
    VkFence* inflight_fncs;
    Buffer* ubufs;
    void** ubuf_mappings;
} RenderResources;

bool make_renderresources(RenderContext* ctx, CleanupStack* cs);

#endif