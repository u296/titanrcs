#include "render.h"
#include "common.h"
#include "vulkan/vulkan_core.h"
#include "commandpool.h"


void drawframe(VkDevice dev, VkSwapchainKHR swapchain, VkExtent2D swapchainextent, VkFramebuffer* fbs, VkSemaphore sem_imgavail, VkFence fen_inflight, VkCommandBuffer cmdbuf) {
    vkWaitForFences(dev, 1, &fen_inflight, VK_TRUE, UINT64_MAX);

    vkResetFences(dev, 1,&fen_inflight);

    u32 i_image = UINT32_MAX;

    vkAcquireNextImageKHR(dev, swapchain, UINT64_MAX, sem_imgavail, VK_NULL_HANDLE, &i_image);

    vkResetCommandBuffer(cmdbuf, 0);

    //recordcommandbuffer(swapchainextent, fbs[i_image], cmdbuf, renderpass, VkPipeline pipeline, struct Error *e_out)

}
