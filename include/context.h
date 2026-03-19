#ifndef CONTEXT_H
#define CONTEXT_H
#include "backend/backend.h"

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
	struct {
		VkSwapchainKHR swpch;
		VkExtent2D swpch_ext;
		u32 n_swpch_img;
		VkImage* swpch_imgs;
		VkImageView* swpch_imgvs;
		VkFramebuffer* fbufs;
	} rendertarget;
	struct {
		VkCommandBuffer* cmd_bufs;
		VkSemaphore* img_ready_sems;
		VkSemaphore* render_finished_sems;
		VkFence* inflight_fncs;
		void** ubuf_mappings;
	} resources;
	struct {
		VkPipeline pipeline;
		VkRenderPass renderpass;
		VkDescriptorSet* desc_sets;
		Renderable the_object;

	} renderobjects;
	
} RenderContext;

#endif