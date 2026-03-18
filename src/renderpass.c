#include "renderpass.h"
#include "cleanupstack.h"
#include "common.h"
#include "vulkan/vulkan_core.h"
#include <volk.h>

typedef struct RenderPassCleanup {
    VkDevice dev;
    VkRenderPass renderpass;
} RenderPassCleanup;

void destroy_renderpass(void* obj) {
    struct RenderPassCleanup* r = (struct RenderPassCleanup*)obj;
    vkDestroyRenderPass(r->dev, r->renderpass, NULL);
}

bool make_renderpass(VkDevice dev, VkFormat swapchainformat, VkRenderPass* renderpass, struct Error* e_out, CleanupStack* cs) {
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swapchainformat;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference car = {};
    car.attachment = 0;
    car.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &car;

    VkSubpassDependency sd = {};
    sd.srcSubpass = VK_SUBPASS_EXTERNAL;
    sd.dstSubpass = 0;
    sd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sd.srcAccessMask = 0;
    sd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sd.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpci = {};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments = &color_attachment;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &sd;

    VkResult r = vkCreateRenderPass(dev, &rpci, NULL, renderpass);

    CLEANUP_START(RenderPassCleanup)
    {dev,*renderpass}
    CLEANUP_END(renderpass)

    VERIFY("renderpass create", r)

    return false;

}

