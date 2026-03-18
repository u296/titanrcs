
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
#include "instance.h"
#include "device.h"
#include "linalg.h"
#include "swapchain.h"
#include "renderpass.h"
#include "pipeline.h"
#include "framebuffers.h"
#include "sync.h"
#include "buffers.h"
#include "vulkan/vulkan_core.h" // having this here doesn't hurt and  prevents intellisense from adding it at the top which would break compilation








constexpr usize WIDTH = 800;
constexpr usize HEIGHT = 600;


bool make_window(GLFWwindow** window) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	*window = glfwCreateWindow(WIDTH, HEIGHT, "TitanRCS", NULL, NULL);

	return false;
}

void destroy_window(void* obj) {
	GLFWwindow** window = (GLFWwindow**)obj;
	glfwDestroyWindow(*window);
	glfwTerminate();
}

#define MAINCHECK    if(f) {\
        printf("Error, code=%d: %s\n",e.code,e.origin);\
        goto fail; \
    }

typedef enum LoopStatus {
	REMAKE_SWAPCHAIN,
	EXIT_PROGRAM
} LoopStatus;

typedef struct RenderContext {
	struct {
		u32 i_current_frame;
		clock_t last_frame_time;
		bool* framebuf_resized;
	} metadata;
	struct {
		u32 max_inflight_frames;
		u32 n_frameratecheck_interval;
	} config;
	struct {
		GLFWwindow* wnd;
		VkInstance inst;
		VkDevice dev;
		Queues queues;
	} backend;
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


void fb_resize_callback(GLFWwindow* wnd, int width, int height) {
	bool* fbresize = (bool*)glfwGetWindowUserPointer(wnd);
	*fbresize = true;
}



int main() {

	struct CleanupStack cs = {};
	cs_init(&cs);

	volkInitialize();
    bool f = false;    

    struct Error e = {.code=0,.origin=""};

	constexpr u32 n_max_inflight = 2;

	GLFWwindow* my_window;
	bool fb_resized = false;
    VkInstance my_instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkSurfaceKHR my_surf;
	VkPhysicalDevice my_physdev;
	VkDevice my_device;
	struct Queues my_queues = {};
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



	make_window(&my_window);
	CLEANUP_START_ONORES(GLFWwindow*)
	my_window
	CLEANUP_END_O(window)

	glfwSetWindowUserPointer(my_window, &fb_resized);
	glfwSetFramebufferSizeCallback(my_window, fb_resize_callback);

	//cs_push(&cs, &my_window, sizeof(my_window), destroy_window);

    f = make_instance(&my_instance, &e, &cs);
	MAINCHECK
	

	volkLoadInstance(my_instance);

	f = make_debugger(my_instance, &debug_messenger, &e, &cs);
	MAINCHECK
	

	f = make_surface(my_instance, my_window, &my_surf, &e, &cs);
	MAINCHECK
	

	f = make_device(my_instance, my_surf, &my_physdev, &my_device, &my_queues, &e, &cs);
	MAINCHECK
	

	volkLoadDevice(my_device);

	CleanupStack swp_cs = {};
	cs_init(&swp_cs);

	f = make_swapchain(my_physdev, my_device, my_queues, my_surf, my_window, &my_swapchain, &swapchain_format, &swapchain_extent,&n_swapchain_images,&swapchain_images, &e, &swp_cs);
	MAINCHECK

	f = make_swapchain_imageviews(my_device, n_swapchain_images, swapchain_images, swapchain_format, &my_imageviews, &e, &swp_cs);
	MAINCHECK

	f = make_renderpass(my_device, swapchain_format, &my_renderpass, &e, &cs);
	MAINCHECK

	f = make_descriptorsetlayout(my_device, &my_desc_set_layout, &cs);
	MAINCHECK

	f = make_graphicspipeline(my_device, swapchain_extent,my_renderpass,my_desc_set_layout, &tri.pipeline_layout,&tri.pipeline,&e,&cs);
	MAINCHECK

	f = make_framebuffers(my_device, swapchain_extent, n_swapchain_images, my_imageviews, my_renderpass, &my_framebuffers, &e,&swp_cs);
	MAINCHECK

	f = make_commandpool(my_device, my_queues, &my_pool, &e, &cs);
	MAINCHECK

	f = make_vertexbuffer(my_physdev, my_device, my_queues, my_pool, &tri.vertexbuf, &e, &cs);
	MAINCHECK

	f = make_indexbuffer(my_physdev, my_device, my_queues, my_pool, &tri.indexbuf, &e, &cs);
	MAINCHECK

	f = make_uniform_buffers(n_max_inflight, my_physdev, my_device, &my_ubufs, &my_ubuf_mappings, &e, &cs);
	MAINCHECK

	f = make_descriptor_pool(n_max_inflight,my_device,&my_dpool,&e,&cs);
	MAINCHECK

	f = make_descriptorsetlayout(my_device, &my_desc_set_layout, &cs);
	MAINCHECK

	f = make_descriptor_sets(n_max_inflight,my_device,my_dpool,my_ubufs,my_desc_set_layout,&my_desc_sets,&e,&cs);
	MAINCHECK

	f = make_commandbuffers(my_device,my_pool,n_max_inflight,&my_commandbufs, &e, &cs);
	MAINCHECK

	f = make_sync_objects(my_device, n_max_inflight,  &sem_imgready, &sem_rendfinish, &fen_inflight, &e,&cs);
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
			&fb_resized
		},
		.config={
			n_max_inflight,
			n_frameratecheck
		},
		.backend={
			my_window,
			my_instance,
			my_device,
			my_queues
		},
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
			
			glfwGetFramebufferSize(my_window, &width, &height);
			while (width == 0 || height == 0) {
				glfwGetFramebufferSize(my_window, &width, &height);
				glfwWaitEvents();
			}

			// if we get here it means the swapchain needs to be recreated
			cs_init(&swp_cs);

			make_swapchain(my_physdev, my_device, my_queues, my_surf, my_window, &my_swapchain, &swapchain_format, &swapchain_extent, &n_swapchain_images, &swapchain_images, &e, &swp_cs);

			make_swapchain_imageviews(my_device, n_swapchain_images, swapchain_images, swapchain_format, &my_imageviews, &e, &swp_cs);

			make_framebuffers(my_device, swapchain_extent, n_swapchain_images, my_imageviews, my_renderpass, &my_framebuffers, &e, &swp_cs);
			printf("remade swapchain\n");
		}
		



		LoopStatus l = do_renderloop(
		&ctx
		);

		vkDeviceWaitIdle(my_device);
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
	

	vkDeviceWaitIdle(my_device);

	cs_consume(&cs);
	return 0;
fail:
	cs_consume(&swp_cs);
	cs_consume(&cs);
	return 1;
    
}