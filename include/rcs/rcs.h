#ifndef RCS_H
#define RCS_H

#include "backend/backend.h"
#include "res.h"
#include "rcs/rcs_pathing.h"
#include <vulkan/vulkan_core.h>

typedef struct RcsPerInflight {
    VkDescriptorSet descset;
    VkDescriptorSet red_descset;
    Image depthimg;
    Image rendtargets[3];
    Buffer ubo;
#ifdef TR_CALCMODE_FFT
    VkDescriptorSet imgtobuf_descset;
    VkDescriptorSet buftoimg_descset;
    Buffer fft_work_buf;
#endif
#ifdef TR_CALCMODE_SUM
    Image intermediate_downscale_img;
#endif
    Image fft_img;
    Buffer extr_buf;
    void* ubufmap;
    VkCommandBuffer cmdbuf;
} RcsPerInflight;

typedef struct RcsResources {
    VkExtent2D ext;
    VkDescriptorPool dpool;
    VkDescriptorSetLayout descset_layout;
    VkPipelineLayout rcs_pipline_layout;
    VkPipeline po_pipeline;
    VkPipeline ptd_pipeline;
    VkPipelineLayout reduction_pipeline_layout;
    VkPipeline reduction_pipeline;
    VkSampler sampler;
    RcsRenderMesh mesh;
    RcsRenderMesh sharp_edge_mesh;
    PathingResources pathres;
#ifdef TR_CALCMODE_FFT
    VkPipelineLayout imgbuftransfer_pipeline_layout;
    VkPipeline imgtobuf_pipeline;
    VkPipeline buftoimg_pipeline;
#endif
#ifdef TR_CALCMODE_SUM
    VkPipelineLayout downscale_pipeline_layout;
    VkPipeline downscale_pipeline;
#endif
    RcsPerInflight sets[N_MAX_INFLIGHT];
} RcsResources;

bool make_rcs_setup(RenderBackend* rb, VkCommandPool cpool,
                    RcsResources* out_res, CleanupStack* cs);

#endif