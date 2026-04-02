#include "rcs/rcs.h"
#include "cleanupstack.h"
#include "cleanupdb.h"
#include "common.h"
#include "rcs/rcs_descset.h"
#include "rcs/rcs_fb.h"
#include "rcs/rcs_imgs.h"
#include "rcs/rcs_pipeline.h"
#include "rcs/rcs_renderpass.h"
#include "rcs/rcs_ubo.h"
#include "rcs/rcs_mesh.h"
#include "res.h"

bool make_rcs_setup(RenderBackend* rb, VkCommandPool cpool, RcsResources* out_res, CleanupStack* cs) {
    Error e;

    constexpr u32 N_RENDTARGETS = 3;
    VkExtent2D ext = {RCS_RESOLUTION, RCS_RESOLUTION};

    VkRenderPass renderpass;
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
    VkFramebuffer rcs_fb;
    void* rcs_ubufmap;
    Renderable rcs_mesh;

    make_rcs_renderpass(rb, &renderpass, &e, cs);

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
    }
    CLEANUP_END(mapping)

    make_rcs_descset(rb, rcs_dpool, rcs_descset_layout, rcs_ubo, &rcs_descset);

    make_rcs_pipeline(rb, ext, rcs_descset_layout, renderpass, &rcs_pipeline_layout, &rcs_pipeline,
                      &e, cs);

    make_rcs_fb(rb, ext, N_RENDTARGETS, rendtargets, rcs_depthimg, renderpass, &rcs_fb, &e,
                cs);

    make_rcs_mesh(rb, cpool, &rcs_mesh.vertexbuf, &rcs_mesh.indexbuf, &rcs_mesh.n_indices, cs);

    RcsResources res = {ext,
                        renderpass,
                        rcs_dpool,
                        rcs_descset_layout,
                        rcs_descset    ,
                        rcs_pipeline_layout,
                        rcs_pipeline,
                        rcs_depthimg,
                        {rendtargets[0], rendtargets[1], rendtargets[2]},
                        rcs_ubo,
                        rcs_fftbuf,
                        rcs_fftimg,
                        rcs_sampler,
                        rcs_ubufmap,
                        rcs_fb,
                        rcs_mesh
                    };

    *out_res = res;
    return false;
}