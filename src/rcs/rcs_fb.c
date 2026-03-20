#include "rcs/rcs_fb.h"
#include "backend/backend.h"
#include "cleanupdb.h"
#include "rcs/rcs_imgs.h"
#include <stdlib.h>



void destroy_framebuffer(void* obj) {
    FramebufferCleanup* fbc = (FramebufferCleanup*)obj;
    vkDestroyFramebuffer(fbc->dev, fbc->framebuffers, NULL);
}

bool make_rcs_fb(RenderBackend* rb, VkExtent2D ext, const u32 n_targets, Image* targets,
                 Image depth, VkRenderPass renderpass, VkFramebuffer* framebuffer,
                 struct Error* e_out, CleanupStack* cs) {
    VkResult r = VK_ERROR_UNKNOWN;

    VkImageView attachments[] = {targets[0].view, targets[1].view, targets[2].view, depth.view};

    VkFramebufferCreateInfo fci = {};
    fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fci.renderPass = renderpass;
    fci.attachmentCount = 4;
    fci.pAttachments = attachments;
    fci.width = ext.width;
    fci.height = ext.height;
    fci.layers = 1;

    


    CLEANUP_START(FramebufferCleanup)
    {rb->dev, *framebuffer}
    CLEANUP_END(framebuffer)

    return false;
}