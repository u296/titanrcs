#ifndef CONTEXT_H
#define CONTEXT_H
#include <time.h>
#include "backend/backend.h"
#include "swapchaincontext/swapchaincontext.h"
#include "resources/renderresources.h"






typedef struct FrameGraph {
	VkPipeline pipeline;
	VkDescriptorSet* desc_sets;
	Renderable the_object;
} FrameGraph;

typedef struct RenderContext {
	struct {
		u32 i_current_frame;
		clock_t last_frame_time;
	} metadata;
	struct {
		u32 max_inflight_frames;
		u32 n_frameratecheck_interval;
	} config;
	RenderBackend backend;
	SwapchainContext swapchain;
	RenderResources resources;
	FrameGraph framegraph;
	
} RenderContext;

#endif