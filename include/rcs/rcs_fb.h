#ifndef RCS_FB_H
#define RCS_FB_H
#include "rcs/rcs_imgs.h"

bool make_rcs_fb(RenderBackend* rb, VkExtent2D ext, const u32 n_targets, Image* targets,
                 Image depth, VkRenderPass renderpass, VkFramebuffer* framebuffer,
                 struct Error* e_out, CleanupStack* cs);

#endif