#ifndef CONTEXT_H
#define CONTEXT_H
#include "common.h"
#include "backend/backend.h"
#include "resources/renderresources.h"
#include "swapchaincontext/swapchaincontext.h"
#include <time.h>
#include "rcs/rcs.h"

typedef struct FrameGraph {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSet* desc_sets;
    RcsRenderMesh the_object;
} FrameGraph;

typedef struct ManualControlState {
    f32 pitch;
    f32 yaw;
    f32 lambda;
    bool active;
} ManualControlState;

typedef struct RenderContext {
    struct {
        u32 i_current_frame;
        struct timespec last_frame_time;
    } metadata;
    struct {
        u32 n_frameratecheck_interval;
        f32 zoom;
    } config;
    RenderBackend backend;
    SwapchainContext swapchain;
    RenderResources resources;
    FrameGraph framegraph;
    RcsResources rcs_resources;
    ManualControlState manual_control;

} RenderContext;

#endif