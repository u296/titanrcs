#ifndef RCS_H
#define RCS_H

#include "backend/backend.h"
#include "res.h"
#include <vulkan/vulkan_core.h>

typedef struct RcsPerInflight {
    VkDescriptorSet descset;
    VkDescriptorSet red_descset;
    Image depthimg;
    Image rendtargets[3];
    Buffer ubo;
    Buffer fft_buf;
    Image fft_img;
    Buffer extr_buf;
    void* ubufmap;
    VkCommandBuffer cmdbuf;
} RcsPerInflight;

typedef struct RcsResources {
    VkExtent2D ext;
    VkDescriptorPool dpool;
    VkDescriptorSetLayout descset_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkPipelineLayout reduction_pipeline_layout;
    VkPipeline reduction_pipeline;
    VkSampler sampler;
    Renderable mesh;
    RcsPerInflight sets[N_MAX_INFLIGHT];
} RcsResources;

bool make_rcs_setup(RenderBackend* rb, VkCommandPool cpool,
                    RcsResources* out_res, CleanupStack* cs);

#endif