#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H
#include "cleanupstack.h"
#include "common.h"
#include "device.h"
#include <GLFW/glfw3.h>




bool make_swapchain(VkPhysicalDevice physdev, VkDevice dev, struct Queues queues, VkSurfaceKHR surf, GLFWwindow* wnd, VkSwapchainKHR* swapchain, VkFormat*swapchain_format,VkExtent2D* swapchain_extent, u32* n_swap_images, VkImage** images, struct Error* e_out, CleanupStack* cs);



bool make_swapchain_imageviews(VkDevice dev, u32 n_swapchain_images, VkImage* images, VkFormat swapchainformat, VkImageView** imageviews, struct Error* e_out, CleanupStack* cs);
#endif