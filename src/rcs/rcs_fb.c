#include "rcs/rcs_fb.h"
#include "backend/backend.h"
#include "cleanupdb.h"
#include "rcs/rcs_imgs.h"
#include <stdlib.h>


bool make_rcs_fb(RenderBackend* rb, VkExtent2D ext, const u32 n_targets, Image* targets, VkRenderPass renderpass, VkFramebuffer** framebuffers, struct Error* e_out, CleanupStack* cs) {
    *framebuffers = malloc(n_targets * sizeof(VkFramebuffer));
    VkResult r = VK_ERROR_UNKNOWN;
    for (u32 i = 0; i < n_targets; i++) {
        VkImageView attachments[] = {
            targets[i].view
        };

        VkFramebufferCreateInfo fci = {};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = renderpass;
        fci.attachmentCount = 1;
        fci.pAttachments = attachments;
        fci.width = ext.width;
        fci.height = ext.height;
        fci.layers = 1;

        r = vkCreateFramebuffer(rb->dev, &fci, NULL, &(*framebuffers)[i]);
        if (r != VK_SUCCESS) {
            break;
        }
    }

    CLEANUP_START(FramebuffersCleanup)
    {rb->dev,*framebuffers,n_targets}
    CLEANUP_END(framebuffers)

    return false;
}