#include "rcs/rcs.h"
#include "cleanupstack.h"
#include "common.h"
#include "rcs/rcs_descset.h"
#include "rcs/rcs_fb.h"
#include "rcs/rcs_imgs.h"
#include "rcs/rcs_pipeline.h"
#include "rcs/rcs_renderpass.h"
#include <vulkan/vulkan_core.h>

bool make_rcs_setup(RenderBackend* rb, RcsResources* out_res, CleanupStack* cs) {
    Error e;

    constexpr u32 N_RENDTARGETS = 3;
    VkExtent2D ext = {200, 200};

    VkRenderPass renderpass;
    VkDescriptorSetLayout rcs_descset_layout;
    VkPipelineLayout rcs_pipeline_layout;
    VkPipeline rcs_pipeline;
    Image rcs_depthimg;
    Image rendtargets[N_RENDTARGETS];
    VkFramebuffer framebuffer;

    make_rcs_renderpass(rb, &renderpass, &e, cs);

    make_rcs_depthresources(rb, ext, &rcs_depthimg, cs);

    make_rcs_rendertargets(rb, ext, N_RENDTARGETS, rendtargets, cs);

    make_rcs_dpool(rb->dev, &out_res->dpool, cs);

    make_rcs_descset_layout(rb->dev, &rcs_descset_layout, cs);

    //make_rcs_descset();

    make_rcs_pipeline(rb, ext, rcs_descset_layout, renderpass, &rcs_pipeline_layout, &rcs_pipeline,
                      &e, cs);

    make_rcs_fb(rb, ext, N_RENDTARGETS, rendtargets, rcs_depthimg, renderpass, &framebuffer, &e,
                cs);

    RcsResources res = {ext,
                        renderpass,
                        out_res->dpool,
                        rcs_descset_layout,
                        rcs_pipeline_layout,
                        rcs_pipeline,
                        rcs_depthimg,
                        {rendtargets[0], rendtargets[1], rendtargets[2]},
                        framebuffer};

    *out_res = res;
    return false;
}