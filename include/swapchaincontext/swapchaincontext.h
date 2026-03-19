#ifndef SWAPCHAINCONTEXT_H
#define SWAPCHAINCONTEXT_H
#include "backend/backend.h"
#include "cleanupstack.h"
#include "common.h"

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

// Abstracts creation of swapchain and related items into a single function
bool make_swapchain_context(
    RenderBackend rb,
    SwapchainContext* swpctx,
    CleanupStack* swp_cs
);


#endif // SWAPCHAINCONTEXT_H
