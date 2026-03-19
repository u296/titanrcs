#ifndef RENDERPASS_H
#define RENDERPASS_H
#include "cleanupstack.h"
#include "common.h"



bool make_renderpass(VkDevice dev, VkFormat swapchainformat, VkRenderPass* renderpass, struct Error* e_out, CleanupStack* cs);


#endif