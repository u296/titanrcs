#ifndef RCS_H
#define RCS_H

#include "backend/backend.h"
#include "res.h"

typedef struct RcsPerInflight {
    VkDescriptorSet descset;
    Image depthimg;
    Image rendtargets[3];
    Buffer ubo;
    Buffer fft_buf;
    Image fft_img;
    void* ubufmap;
    VkCommandBuffer cmdbuf;
} RcsPerInflight;

typedef struct RcsResources {
    VkExtent2D ext;
    VkDescriptorPool dpool;
    VkDescriptorSetLayout descset_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkSampler sampler;
    Renderable mesh;
    RcsPerInflight sets[N_MAX_INFLIGHT];
} RcsResources;

bool make_rcs_setup(RenderBackend* rb, VkCommandPool cpool,
                    RcsResources* out_res, CleanupStack* cs);

#endif