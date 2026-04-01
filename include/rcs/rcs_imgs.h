#ifndef RCS_DEPTHIMG_H
#define RCS_DEPTHIMG_H

#include "backend/backend.h"
#include <vulkan/vulkan_core.h>

/*
typedef struct {
    VkImage img;
    VkImageView view;
    VmaAllocation alloc;
} Image;
 */

bool make_rcs_depthresources(RenderBackend* rb, VkExtent2D ext, Image* depthimg, CleanupStack* cs);

bool make_rcs_rendertargets(RenderBackend* rb, VkExtent2D ext, const u32 n_targets,
                            Image* rendtargets, CleanupStack* cs);

bool make_sampler(RenderBackend* rb, VkSampler* out_sampler, CleanupStack* cs);

#endif