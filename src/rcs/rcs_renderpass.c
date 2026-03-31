#include "backend/backend.h"
#include "common.h"

VkFormat find_depth_format(VkPhysicalDevice physdev) {
    // don't know if this is allowed
    return VK_FORMAT_D32_SFLOAT;
}

void destroy_renderpass(void* obj);

typedef struct RenderPassCleanup {
    VkDevice dev;
    VkRenderPass renderpass;
} RenderPassCleanup;

bool make_rcs_renderpass(RenderBackend* rb, VkRenderPass* renderpass, struct Error* e_out,
                         CleanupStack* cs) {

    // VK_FORMAT_R32G32B32A32_SFLOAT;

    // three of these will be rendered to by the rcs shader
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = VK_FORMAT_R32G32B32A32_SFLOAT; // don't know if this is allowed
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment = {};
    depth_attachment.format = find_depth_format(rb->physdev);
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference car0 = {};
    car0.attachment = 0;
    car0.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription desc0 = color_attachment;
    desc0.format = VK_FORMAT_R32G32_SFLOAT;

    VkAttachmentReference car1 = {};
    car1.attachment = 1;
    car1.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference car2 = {};
    car2.attachment = 2;
    car2.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference dar = {};
    dar.attachment = 3; // index 3 since we have 3 color attachments (0..2)
    dar.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_refs[3] = {car0, car1, car2};

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 3;
    subpass.pColorAttachments = color_refs;
    subpass.pDepthStencilAttachment = &dar;

    VkSubpassDependency sd = {};
    sd.srcSubpass = VK_SUBPASS_EXTERNAL;
    sd.dstSubpass = 0;
    sd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    sd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    sd.srcAccessMask = 0;
    sd.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpci = {};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 4;
    rpci.pAttachments = (VkAttachmentDescription[]){desc0, color_attachment,
                                                    color_attachment, depth_attachment};
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &sd;

    VkResult r = vkCreateRenderPass(rb->dev, &rpci, NULL, renderpass);

    CLEANUP_START(RenderPassCleanup){rb->dev, *renderpass} CLEANUP_END(renderpass)

        VERIFY("renderpass create", r)

            return false;
}