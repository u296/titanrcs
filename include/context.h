#ifndef CONTEXT_H
#define CONTEXT_H
#include "backend/backend.h"
#include "resources/renderresources.h"
#include "swapchaincontext/swapchaincontext.h"
#include <time.h>
#include "rcs/rcs.h"

typedef struct FrameGraph {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSet* desc_sets;
    Renderable the_object;
} FrameGraph;

typedef struct RenderContext {
    struct {
        u32 i_current_frame;
        clock_t last_frame_time;
    } metadata;
    struct {
        u32 n_frameratecheck_interval;
    } config;
    RenderBackend backend;
    SwapchainContext swapchain;
    RenderResources resources;
    FrameGraph framegraph;
    RcsResources rcs_resources;


} RenderContext;

#endif