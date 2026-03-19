
#include <string.h>
#include<volk.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<time.h>
#include<GLFW/glfw3.h>


#include "commandpool.h"
#include "common.h"
#include "cleanupstack.h"
#include "descriptors.h"
#include "linalg.h"
#include "swapchain.h"
#include "renderpass.h"
#include "pipeline.h"
#include "framebuffers.h"
#include "sync.h"
#include "buffers.h"
#include "vulkan/vulkan_core.h" // having this here doesn't hurt and  prevents intellisense from adding it at the top which would break compilation

#include "backend/backend.h"



#define MAINCHECK    if(f) {\
        printf("Error, code=%d: %s\n",e.code,e.origin);\
        goto fail; \
    }

typedef enum LoopStatus {
	REMAKE_SWAPCHAIN,
	EXIT_PROGRAM
} LoopStatus;

#include "context.h"

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
    	VkResult img_ac_res = vkAcquireNextImageKHR(ctx->backend.dev, ctx->rendertarget.swpch, UINT64_MAX, ctx->resources.img_ready_sems[i_frame_modn], VK_NULL_HANDLE, &i_image);

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

		update_uniformbuffer(ctx->metadata.i_current_frame, ctx->rendertarget.swpch_ext, ctx->resources.ubuf_mappings[i_frame_modn]);

    	f = recordcommandbuffer(ctx->rendertarget.swpch_ext, ctx->rendertarget.fbufs[i_image], ctx->resources.cmd_bufs[i_frame_modn], ctx->renderobjects.renderpass, ctx->renderobjects.desc_sets[i_frame_modn], ctx->renderobjects.the_object, &e);
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
		pi.pSwapchains = &ctx->rendertarget.swpch;
		pi.pImageIndices = &i_image;
		pi.pResults = NULL;

		VkResult pres_res = vkQueuePresentKHR(ctx->backend.queues.present_queue, &pi);

		switch (pres_res) {
			case VK_SUCCESS:
				break;
			case VK_SUBOPTIMAL_KHR:
			case VK_ERROR_OUT_OF_DATE_KHR:
				*ctx->metadata.framebuf_resized = false;
				return REMAKE_SWAPCHAIN;
				break;
			default:
				goto fail;
				break;
		}

		if (*ctx->metadata.framebuf_resized) {
			*ctx->metadata.framebuf_resized = false;
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

	struct CleanupStack cs = {};
	cs_init(&cs);

	volkInitialize();
    bool f = false;    

    struct Error e = {.code=0,.origin=""};

	constexpr u32 n_max_inflight = 2;

	RenderBackend my_rendbackend;
	VkSwapchainKHR my_swapchain;
	VkFormat swapchain_format;
	VkExtent2D swapchain_extent;
	u32 n_swapchain_images = 0;
	VkImage* swapchain_images;
	VkImageView* my_imageviews;
	VkRenderPass my_renderpass;
	VkDescriptorSetLayout my_desc_set_layout;
	Renderable tri;
	Buffer* my_ubufs;
	void** my_ubuf_mappings;
	VkDescriptorPool my_dpool;
	VkDescriptorSet* my_desc_sets;
	VkFramebuffer* my_framebuffers;
	VkCommandPool my_pool;
	VkCommandBuffer* my_commandbufs;
	VkSemaphore* sem_imgready;
	VkSemaphore* sem_rendfinish;
	VkFence* fen_inflight;



	init_backend(&my_rendbackend, &cs);

	CleanupStack swp_cs = {};
	cs_init(&swp_cs);

	f = make_swapchain(my_rendbackend.physdev, my_rendbackend.dev, my_rendbackend.queues, my_rendbackend.surf, my_rendbackend.wnd, &my_swapchain, &swapchain_format, &swapchain_extent,&n_swapchain_images,&swapchain_images, &e, &swp_cs);
	MAINCHECK

	f = make_swapchain_imageviews(my_rendbackend.dev, n_swapchain_images, swapchain_images, swapchain_format, &my_imageviews, &e, &swp_cs);
	MAINCHECK

	f = make_renderpass(my_rendbackend.dev, swapchain_format, &my_renderpass, &e, &cs);
	MAINCHECK

	f = make_descriptorsetlayout(my_rendbackend.dev, &my_desc_set_layout, &cs);
	MAINCHECK

	f = make_graphicspipeline(my_rendbackend.dev, swapchain_extent,my_renderpass,my_desc_set_layout, &tri.pipeline_layout,&tri.pipeline,&e,&cs);
	MAINCHECK

	f = make_framebuffers(my_rendbackend.dev, swapchain_extent, n_swapchain_images, my_imageviews, my_renderpass, &my_framebuffers, &e,&swp_cs);
	MAINCHECK

	f = make_commandpool(my_rendbackend.dev, my_rendbackend.queues, &my_pool, &e, &cs);
	MAINCHECK

	f = make_vertexbuffer(my_rendbackend.physdev, my_rendbackend.dev, my_rendbackend.queues, my_pool, &tri.vertexbuf, &e, &cs);
	MAINCHECK

	f = make_indexbuffer(my_rendbackend.physdev, my_rendbackend.dev, my_rendbackend.queues, my_pool, &tri.indexbuf, &e, &cs);
	MAINCHECK

	f = make_uniform_buffers(n_max_inflight, my_rendbackend.physdev, my_rendbackend.dev, &my_ubufs, &my_ubuf_mappings, &e, &cs);
	MAINCHECK

	f = make_descriptor_pool(n_max_inflight,my_rendbackend.dev,&my_dpool,&e,&cs);
	MAINCHECK

	f = make_descriptorsetlayout(my_rendbackend.dev, &my_desc_set_layout, &cs);
	MAINCHECK

	f = make_descriptor_sets(n_max_inflight,my_rendbackend.dev,my_dpool,my_ubufs,my_desc_set_layout,&my_desc_sets,&e,&cs);
	MAINCHECK

	f = make_commandbuffers(my_rendbackend.dev,my_pool,n_max_inflight,&my_commandbufs, &e, &cs);
	MAINCHECK

	f = make_sync_objects(my_rendbackend.dev, n_max_inflight,  &sem_imgready, &sem_rendfinish, &fen_inflight, &e,&cs);
	MAINCHECK

	u64 i_frame = 0;
	constexpr u64 n_frameratecheck = 100;

	clock_t last_time = clock();

	bool firstiter = true;

	bool shouldclose = false;

	// make rendercontext here

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
		.rendertarget={
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
		.renderobjects={
			tri.pipeline,
			my_renderpass,
			my_desc_sets,
			tri
		}
	};

	do {
		// this is the loop that owns the swapchain and recreates it whenever the renderloop exits because the swapchain needs renewal

		if (!firstiter) {

			int width = 0, height = 0;
			
			glfwGetFramebufferSize(my_rendbackend.wnd, &width, &height);
			while (width == 0 || height == 0) {
				glfwGetFramebufferSize(my_rendbackend.wnd, &width, &height);
				glfwWaitEvents();
			}

			// if we get here it means the swapchain needs to be recreated
			cs_init(&swp_cs);

			make_swapchain(my_rendbackend.physdev, my_rendbackend.dev, my_rendbackend.queues, my_rendbackend.surf, my_rendbackend.wnd, &my_swapchain, &swapchain_format, &swapchain_extent, &n_swapchain_images, &swapchain_images, &e, &swp_cs);

			make_swapchain_imageviews(my_rendbackend.dev, n_swapchain_images, swapchain_images, swapchain_format, &my_imageviews, &e, &swp_cs);

			make_framebuffers(my_rendbackend.dev, swapchain_extent, n_swapchain_images, my_imageviews, my_renderpass, &my_framebuffers, &e, &swp_cs);
			printf("remade swapchain\n");
		}
		



		LoopStatus l = do_renderloop(
		&ctx
		);

		vkDeviceWaitIdle(my_rendbackend.dev);
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
	

	vkDeviceWaitIdle(my_rendbackend.dev);

	cs_consume(&cs);
	return 0;
fail:
	cs_consume(&swp_cs);
	cs_consume(&cs);
	return 1;
    
}