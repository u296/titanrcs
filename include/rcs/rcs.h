#ifndef RCS_H
#define RCS_H

#include "backend/backend.h"
#include <vulkan/vulkan_core.h>

#include "rcs/rcs_imgs.h"

typedef struct RcsResources {
    VkExtent2D ext;
    VkRenderPass renderpass;
    VkDescriptorSetLayout descset_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    Image depthimg;
    Image rendtargets[3];
    VkFramebuffer framebuffer;

} RcsResources;

bool make_rcs_setup(RenderBackend* rb, RcsResources* out_res, CleanupStack* cs);



#endif