#include "swapchain.h"
#include "cleanupstack.h"
#include "common.h"
#include "vulkan/vulkan_core.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>







typedef struct SwapchainCleanup {
	VkDevice dev;
	VkSwapchainKHR swapchain;
	VkImage* images;
} SwapchainCleanup;

void destroy_swapchain(void* obj) {
	struct SwapchainCleanup* s = (struct SwapchainCleanup*)obj;
	vkDestroySwapchainKHR(s->dev, s->swapchain, NULL);
	free(s->images);
}

bool make_swapchain(
	VkPhysicalDevice physdev,
	VkDevice dev,
	Queues queues,
	VkSurfaceKHR surf,
	GLFWwindow* wnd,
	VkSwapchainKHR* swapchain,
	VkFormat*swapchain_format,
	VkExtent2D* swapchain_extent,
	u32* n_swap_images,
	VkImage** images,
	struct Error* e_out,
	CleanupStack* cs
) {
	VkSurfaceCapabilitiesKHR surfcap = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physdev, surf, &surfcap);

	u32 n_formats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physdev, surf, &n_formats, NULL);
	VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * n_formats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physdev, surf, &n_formats, formats);


	u32 n_presmodes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physdev, surf, &n_presmodes, NULL);
	VkPresentModeKHR* presmodes = malloc(sizeof(VkPresentModeKHR) * n_presmodes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physdev, surf, &n_presmodes, presmodes);

	u8 set_format=0;
	VkSurfaceFormatKHR chosen_format;

	for (u32 i = 0; i < n_formats; i++) {
		if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			chosen_format = formats[i];
		}
	}

	if (!set_format) {
		chosen_format = formats[0];
	}

	VkPresentModeKHR chosen_presentmode = VK_PRESENT_MODE_FIFO_KHR;

	VkExtent2D sw_ext = {};

	if (surfcap.currentExtent.width != UINT32_MAX) {
		sw_ext = surfcap.currentExtent;
	} else {
		i32 w,h;
		glfwGetFramebufferSize(wnd, &w, &h);
		sw_ext.width = (u32)CLAMP(w,surfcap.minImageExtent.width,surfcap.maxImageExtent.width);
		sw_ext.height = (u32)CLAMP(h,surfcap.minImageExtent.height,surfcap.maxImageExtent.height);
	}


	free(formats);
	free(presmodes);

	u32 n_images = surfcap.minImageCount;

	if (surfcap.maxImageCount == 0) {
		n_images += 1;
	} else {
		n_images = CLAMP(n_images+1, surfcap.minImageCount, surfcap.maxImageCount);
	}

	VkSwapchainCreateInfoKHR sci = {};
	sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sci.surface = surf;
	sci.minImageCount = n_images;
	sci.imageFormat = chosen_format.format;
	sci.imageColorSpace = chosen_format.colorSpace;
	sci.imageExtent = sw_ext;
	sci.imageArrayLayers = 1;
	sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	u32 qfi[2] = {queues.i_graphics_queue_fam, queues.i_present_queue_fam};

	if (qfi[0] == qfi[1]) {
		sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		sci.queueFamilyIndexCount = 0;
		sci.pQueueFamilyIndices = NULL;
	} else {
		sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		sci.queueFamilyIndexCount = 2;
		sci.pQueueFamilyIndices = qfi;
	}

	sci.preTransform = surfcap.currentTransform;
	sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	sci.presentMode = chosen_presentmode;
	sci.clipped = VK_TRUE;
	sci.oldSwapchain = VK_NULL_HANDLE;

	VkResult r = vkCreateSwapchainKHR(dev,&sci,NULL, swapchain);


	if (r == VK_SUCCESS) {
		vkGetSwapchainImagesKHR(dev, *swapchain,n_swap_images, NULL);
		*images = malloc(*n_swap_images * sizeof(VkImage));
		vkGetSwapchainImagesKHR(dev, *swapchain,n_swap_images, *images);
	}
	
	CLEANUP_START(SwapchainCleanup)
	{dev, *swapchain, *images}
	CLEANUP_END(swapchain)

	VERIFY("swapchain create", r);
	printf("made swapchain with extents %ux%u\n", sci.imageExtent.width,sci.imageExtent.height);

	*swapchain_extent = sw_ext;
	*swapchain_format = chosen_format.format;

	
	

	return false;
}

typedef struct ImageViewCleanup {
    VkDevice dev;
    VkImageView* views;
    u32 n_imageviews;
} ImageViewCleanup;

void destroy_imageviews(void* obj) {
	struct ImageViewCleanup* s = (struct ImageViewCleanup*)obj;
	for (u32 i = 0; i < s->n_imageviews; i++) {
        vkDestroyImageView(s->dev, s->views[i], NULL);
    }
    free(s->views);
}

bool make_swapchain_imageviews(VkDevice dev, u32 n_swapchain_images, VkImage* images, VkFormat swapchainformat, VkImageView** imageviews, struct Error* e_out, CleanupStack* cs) {
    *imageviews = malloc(n_swapchain_images * sizeof(VkImageView));

    VkResult r = VK_ERROR_UNKNOWN;

    for (u32 i = 0; i < n_swapchain_images; i++) {
        VkImageViewCreateInfo vci = {};
        vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image = images[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = swapchainformat;
        vci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.baseMipLevel = 0;
        vci.subresourceRange.levelCount = 1;
        vci.subresourceRange.baseArrayLayer = 0;
        vci.subresourceRange.layerCount = 1;

        r = vkCreateImageView(dev, &vci, NULL, &(*imageviews)[i]);

		

        if ( r != VK_SUCCESS) {
			break;
		}
    }

	CLEANUP_START(ImageViewCleanup)
	{dev,*imageviews,n_swapchain_images}
	CLEANUP_END(imageviews)

	VERIFY("image views", r)

    return false;
}

