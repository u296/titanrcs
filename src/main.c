
#include <string.h>
#include<volk.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<time.h>
#include<GLFW/glfw3.h>


#include "common.h"
#include "cleanupstack.h"
#include "descriptors.h"
#include "linalg.h"

#include "pipeline.h"
#include "buffers.h"
#include "resources/renderresources.h"
#include "vulkan/vulkan_core.h" // having this here doesn't hurt and  prevents intellisense from adding it at the top which would break compilation



#define MAINCHECK    if(f) {\
        printf("Error, code=%d: %s\n",e.code,e.origin);\
        goto fail; \
    }

typedef enum LoopStatus {
	REMAKE_SWAPCHAIN,
	EXIT_PROGRAM
} LoopStatus;

#include "context.h"

bool recordcommandbuffer(VkExtent2D swapchainextent, VkFramebuffer fb, VkCommandBuffer cmdbuf, VkRenderPass renderpass, VkDescriptorSet desc_set, Renderable ren, struct Error* e_out);


LoopStatus do_renderloop(
	RenderContext* ctx
) {
	VkResult f = VK_ERROR_UNKNOWN;
	struct Error e = {};

	while (!glfwWindowShouldClose(ctx->backend.wnd)) {
		glfwPollEvents();


		const u64 i_frame_modn = ctx->metadata.i_current_frame % ctx->config.max_inflight_frames;

		vkWaitForFences(ctx->backend.dev, 1, &ctx->resources.inflight_fncs[i_frame_modn], VK_TRUE, UINT32_MAX);

    	u32 i_image = UINT32_MAX;
    	VkResult img_ac_res = vkAcquireNextImageKHR(ctx->backend.dev, ctx->swapchain.swpch, UINT64_MAX, ctx->resources.img_ready_sems[i_frame_modn], VK_NULL_HANDLE, &i_image);

		switch (img_ac_res) {
			case VK_ERROR_OUT_OF_DATE_KHR:
				return REMAKE_SWAPCHAIN;
				break;
			case VK_SUBOPTIMAL_KHR:
			case VK_SUCCESS:
				break;
			default:
				goto fail;
				break;
		}

		vkResetFences(ctx->backend.dev, 1,&ctx->resources.inflight_fncs[i_frame_modn]);
    	vkResetCommandBuffer(ctx->resources.cmd_bufs[i_frame_modn], 0);

		update_uniformbuffer(ctx->metadata.i_current_frame, ctx->swapchain.swpch_ext, ctx->resources.ubuf_mappings[i_frame_modn]);

    	f = recordcommandbuffer(ctx->swapchain.swpch_ext, ctx->swapchain.fbufs[i_image], ctx->resources.cmd_bufs[i_frame_modn], ctx->swapchain.renderpass, ctx->framegraph.desc_sets[i_frame_modn], ctx->framegraph.the_object, &e);
		MAINCHECK
		

		VkSemaphore waitsems[1] = {
			ctx->resources.img_ready_sems[i_frame_modn]
		};
		VkSemaphore signalsems[1] = {
			ctx->resources.render_finished_sems[i_frame_modn]
		};
		VkPipelineStageFlags waitstages[1] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		VkSubmitInfo si = {};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.waitSemaphoreCount = 1;
		si.pWaitSemaphores = waitsems;
		si.pWaitDstStageMask = waitstages;
		si.commandBufferCount = 1;
		si.pCommandBuffers = &ctx->resources.cmd_bufs[i_frame_modn];
		si.signalSemaphoreCount = 1;
		si.pSignalSemaphores = signalsems;


		vkQueueSubmit(ctx->backend.queues.graphics_queue, 1, &si, ctx->resources.inflight_fncs[i_frame_modn]);

		VkPresentInfoKHR pi = {};
		pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pi.waitSemaphoreCount = 1;
		pi.pWaitSemaphores = signalsems;
		pi.swapchainCount = 1;
		pi.pSwapchains = &ctx->swapchain.swpch;
		pi.pImageIndices = &i_image;
		pi.pResults = NULL;

		VkResult pres_res = vkQueuePresentKHR(ctx->backend.queues.present_queue, &pi);

		switch (pres_res) {
			case VK_SUCCESS:
				break;
			case VK_SUBOPTIMAL_KHR:
			case VK_ERROR_OUT_OF_DATE_KHR:
				ctx->backend.fb_resized = false;
				return REMAKE_SWAPCHAIN;
				break;
			default:
				goto fail;
				break;
		}

		if (ctx->backend.fb_resized) {
			ctx->backend.fb_resized = false;
			return REMAKE_SWAPCHAIN;
		}

		(ctx->metadata.i_current_frame)++;

		if (ctx->metadata.i_current_frame % ctx->config.n_frameratecheck_interval == ctx->config.n_frameratecheck_interval-1) {
			const clock_t time_now = clock();

			const float fps = (float)ctx->config.n_frameratecheck_interval * (float)CLOCKS_PER_SEC / (float)(time_now - ctx->metadata.last_frame_time);

			ctx->metadata.last_frame_time = time_now;

			printf("FPS: %.0f\n", fps);
		}

	}
fail: 
	return EXIT_PROGRAM;
}



int main() {

	RenderContext ctx = {};

	struct CleanupStack cs = {};
	cs_init(&cs);

	volkInitialize();
    bool f = false;    

    struct Error e = {.code=0,.origin=""};

	constexpr u32 n_max_inflight = 2;

	//RenderBackend my_rendbackend;

	//SwapchainContext my_swpctx;

	VkDescriptorSetLayout my_desc_set_layout;
	Renderable tri;
	VkDescriptorPool my_dpool;



	init_backend(&ctx.backend, &cs);

	CleanupStack swp_cs = {};
	cs_init(&swp_cs);

	make_swapchain_context(ctx.backend, &ctx.swapchain, &swp_cs);

	
	make_renderresources(&ctx, &cs);

	f = make_descriptorsetlayout(ctx.backend.dev, &my_desc_set_layout, &cs);
	MAINCHECK

	f = make_graphicspipeline(ctx.backend.dev, ctx.swapchain.swpch_ext,ctx.swapchain.renderpass,my_desc_set_layout, &tri.pipeline_layout,&tri.pipeline,&e,&cs);
	MAINCHECK

	

	

	f = make_vertexbuffer(ctx.backend.physdev, ctx.backend.dev, ctx.backend.queues, ctx.resources.cmd_pool, &tri.vertexbuf, &e, &cs);
	MAINCHECK

	f = make_indexbuffer(ctx.backend.physdev, ctx.backend.dev, ctx.backend.queues, ctx.resources.cmd_pool, &tri.indexbuf, &e, &cs);
	MAINCHECK

	

	f = make_descriptor_pool(n_max_inflight,ctx.backend.dev,&my_dpool,&e,&cs);
	MAINCHECK

	f = make_descriptorsetlayout(ctx.backend.dev, &my_desc_set_layout, &cs);
	MAINCHECK

	f = make_descriptor_sets(n_max_inflight,ctx.backend.dev,my_dpool,ctx.resources.ubufs,my_desc_set_layout,&ctx.framegraph.desc_sets,&e,&cs);
	MAINCHECK

	

	

	//u64 i_frame = 0;
	constexpr u64 n_frameratecheck = 100;

	clock_t last_time = clock();

	bool firstiter = true;

	bool shouldclose = false;

	// make rendercontext here
/*
	RenderContext ctx = {
		.metadata={
			0,
			last_time,
		},
		.config={
			n_max_inflight,
			n_frameratecheck
		},
		.backend = my_rendbackend,
		.swapchain={
			my_swapchain,
			swapchain_extent,
			n_swapchain_images,
			swapchain_images,
			my_imageviews,
			my_framebuffers
		},
		.resources={
			my_commandbufs,
			sem_imgready,
			sem_rendfinish,
			fen_inflight,
			my_ubuf_mappings
		},
		.framegraph={
			tri.pipeline,
			my_renderpass,
			my_desc_sets,
			tri
		}
	};*/

	ctx.metadata.i_current_frame = 0;
	ctx.metadata.last_frame_time = last_time;
	ctx.config.max_inflight_frames = n_max_inflight;
	ctx.config.n_frameratecheck_interval = n_frameratecheck;

	//ctx.backend = my_rendbackend;
	//ctx.swapchain = my_swpctx;
	ctx.framegraph.pipeline=tri.pipeline;
	ctx.framegraph.the_object=tri;



	do {
		// this is the loop that owns the swapchain and recreates it whenever the renderloop exits because the swapchain needs renewal

		if (!firstiter) {

			int width = 0, height = 0;
			
			glfwGetFramebufferSize(ctx.backend.wnd, &width, &height);
			while (width == 0 || height == 0) {
				glfwGetFramebufferSize(ctx.backend.wnd, &width, &height);
				glfwWaitEvents();
			}

			// if we get here it means the swapchain needs to be recreated
			cs_init(&swp_cs);

			make_swapchain_context(ctx.backend, &ctx.swapchain, &swp_cs);
		}

		LoopStatus l = do_renderloop(&ctx);

		cs_consume(&swp_cs);

		switch (l) {
		case REMAKE_SWAPCHAIN:
			break;
		case EXIT_PROGRAM:
			shouldclose = true;
			break;
		
		}

		firstiter = false;
	} while (!shouldclose);
	

	vkDeviceWaitIdle(ctx.backend.dev);

	cs_consume(&cs);
	return 0;
fail:
	cs_consume(&swp_cs);
	cs_consume(&cs);
	return 1;
    
}