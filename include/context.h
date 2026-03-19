#ifndef CONTEXT_H
#define CONTEXT_H
#include "backend/backend.h"
#include <vulkan/vulkan_core.h>


typedef struct SwapchainContext {
	VkSwapchainKHR swpch;
	VkExtent2D swpch_ext;
	u32 n_swpch_img;
	VkFormat format;
	VkImage* swpch_imgs;
	VkImageView* swpch_imgvs;
	VkRenderPass renderpass;
	VkFramebuffer* fbufs;
} SwapchainContext;

typedef struct RenderResources {
	VkCommandBuffer* cmd_bufs;
	VkSemaphore* img_ready_sems;
	VkSemaphore* render_finished_sems;
	VkFence* inflight_fncs;
	void** ubuf_mappings;
} RenderResources;

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