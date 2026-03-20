#include "rcs/rcs.h"
#include "cleanupstack.h"
#include "common.h"
#include "rcs/rcs_fb.h"
#include "rcs/rcs_imgs.h"
#include "rcs/rcs_pipeline.h"
#include "rcs/rcs_renderpass.h"
#include <vulkan/vulkan_core.h>

typedef struct DescriptorSetLayoutCleanup {
    VkDevice dev;
    VkDescriptorSetLayout desc_layout;
} DescriptorSetLayoutCleanup;

void destroy_descriptor_set_layout(void* obj);

bool make_rcs_descset_layout(VkDevice dev, VkDescriptorSetLayout* desc_layout, CleanupStack* cs) {
    VkDescriptorSetLayoutBinding dslb = {};
    dslb.binding = 0;
    dslb.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dslb.descriptorCount = 1;
    dslb.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    dslb.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo dsli = {};
    dsli.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsli.bindingCount = 1;
    dsli.pBindings = &dslb;

    VkResult r = vkCreateDescriptorSetLayout(dev, &dsli, NULL, desc_layout);
    CLEANUP_START(DescriptorSetLayoutCleanup){dev, *desc_layout} CLEANUP_END(descriptor_set_layout)

        return false;
}

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

    make_rcs_descset_layout(rb->dev, &rcs_descset_layout, cs);

    make_rcs_fb(rb, ext, N_RENDTARGETS, rendtargets, rcs_depthimg, renderpass, &framebuffer, &e, cs);

    RcsResources res = {ext,
                        renderpass,
                        rcs_descset_layout,
                        rcs_pipeline_layout,
                        rcs_pipeline,
                        rcs_depthimg,
                        {rendtargets[0], rendtargets[1], rendtargets[2]},
                       framebuffer};

    *out_res = res;
    return false;
}