#include "rcs/rcs.h"
#include "cleanupdb.h"
#include "cleanupstack.h"
#include "common.h"
#include "rcs/rcs_cmdbuf.h"
#include "rcs/rcs_descset.h"
#include "rcs/rcs_imgs.h"
#include "rcs/rcs_mesh.h"
#include "rcs/rcs_pipeline.h"
#include "rcs/rcs_ubo.h"
#include "res.h"

bool make_rcs_setup(RenderBackend* rb, VkCommandPool cpool,
                    RcsResources* out_res, CleanupStack* cs) {
    Error e;

    constexpr u32 N_RENDTARGETS = 3;
    VkExtent2D ext = {RCS_RESOLUTION, RCS_RESOLUTION};

    VkDescriptorPool rcs_dpool;
    VkDescriptorSetLayout rcs_descset_layout;
    VkPipelineLayout rcs_pipeline_layout;
    VkPipeline rcs_pipeline;
    Image rcs_depthimg = {};
    Image rendtargets[N_RENDTARGETS] = {};
    Buffer rcs_ubo = {};
    Buffer rcs_fftbuf = {};
    Image rcs_fftimg = {};
    VkSampler rcs_sampler;
    VkDescriptorSet rcs_descset;
    VkCommandBuffer rcs_cmdbuf;
    void* rcs_ubufmap;
    Renderable rcs_mesh;

    VkFormat col_formats[3] = {VK_FORMAT_R32G32_SFLOAT,
                               VK_FORMAT_R32G32B32A32_SFLOAT,
                               VK_FORMAT_R32G32B32A32_SFLOAT};

    VkFormat depth_format = VK_FORMAT_D32_SFLOAT;

    make_rcs_depthresources(rb, ext, &rcs_depthimg, cs);

    make_rcs_rendertargets(rb, ext, N_RENDTARGETS, rendtargets, cs);

    make_rcs_dpool(rb->dev, &rcs_dpool, cs);

    make_rcs_descset_layout(rb->dev, &rcs_descset_layout, cs);

    make_rcs_ubo(rb, &rcs_ubo, cs);

    make_rcs_fftbuf(rb, &rcs_fftbuf, cs);

    make_rcs_fftimg(rb, ext, &rcs_fftimg, cs);

    make_sampler(rb, &rcs_sampler, cs);

    vmaMapMemory(rb->alloc, rcs_ubo.alloc, &rcs_ubufmap);
    CLEANUP_START_NORES(MappingCleanup){
        .allocctx = rb->alloc,
        .alloc = rcs_ubo.alloc,
    } CLEANUP_END(mapping)

        make_rcs_descset(rb, rcs_dpool, rcs_descset_layout, rcs_ubo,
                         &rcs_descset);

    make_rcs_pipeline(rb, ext, rcs_descset_layout, col_formats, &depth_format,
                      &rcs_pipeline_layout, &rcs_pipeline, &e, cs);

    make_rcs_cmdbuf(rb, cpool, &rcs_cmdbuf, cs);

    make_rcs_mesh(rb, cpool, &rcs_mesh.vertexbuf, &rcs_mesh.indexbuf,
                  &rcs_mesh.n_indices, cs);

    RcsResources res = {ext,
                        rcs_dpool,
                        rcs_descset_layout,
                        rcs_descset,
                        rcs_pipeline_layout,
                        rcs_pipeline,
                        rcs_depthimg,
                        {rendtargets[0], rendtargets[1], rendtargets[2]},
                        rcs_ubo,
                        rcs_fftbuf,
                        rcs_fftimg,
                        rcs_sampler,
                        rcs_ubufmap,
                        rcs_cmdbuf,
                        rcs_mesh};

    *out_res = res;
    return false;
}