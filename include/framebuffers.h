#ifndef FRAMEBUFFERS_H
#define FRAMEBUFFERS_H
#include "cleanupstack.h"
#include "common.h"
#include "vulkan/vulkan_core.h"





bool make_framebuffers(VkDevice dev, VkExtent2D swapchainextent, u32 n_swapchain_img, VkImageView* swapchain_imgviews, VkRenderPass renderpass, VkFramebuffer** framebuffers, struct Error* e_out,CleanupStack*cs);



#endif