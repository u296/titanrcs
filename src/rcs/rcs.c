#include "rcs/rcs.h"
#include "cleanupdb.h"
#include "cleanupstack.h"
#include "common.h"
#include "rcs/rcs_cmdbuf.h"
#include "rcs/rcs_descset.h"
#include "rcs/rcs_imgs.h"
#include "rcs/rcs_mesh.h"
#include "rcs/rcs_pathing.h"
#include "rcs/rcs_pipeline.h"
#include "rcs/rcs_ubo.h"
#include "res.h"

bool make_rcs_setup(RenderBackend* rb, VkCommandPool cpool,
                    RcsResources* out_res, CleanupStack* cs) {
    Error e;

    constexpr u32 N_RENDTARGETS = 3;
    VkExtent2D ext = {RCS_RESOLUTION, RCS_RESOLUTION};

    PathingResources pathres = {};
    VkDescriptorPool rcs_dpool;
    VkDescriptorSetLayout rcs_descset_layout;
    VkPipelineLayout rcs_pipeline_layout;
    VkPipeline rcs_pipeline;
    VkDescriptorSetLayout rcs_red_descset_layout;
    VkPipelineLayout rcs_red_pipeline_layout;
    VkPipeline rcs_red_pipeline;
    VkSampler rcs_sampler;
    Renderable rcs_mesh;
    RcsPerInflight rcs_inflights[N_MAX_INFLIGHT];

    VkFormat col_formats[3] = {VK_FORMAT_R32G32_SFLOAT,
                               VK_FORMAT_R8G8B8A8_SRGB,
                               VK_FORMAT_R8G8B8A8_SRGB};

    VkFormat depth_format = VK_FORMAT_D32_SFLOAT;

    make_rcs_pathingresources(&pathres, cs);

    make_rcs_dpool(rb->dev, &rcs_dpool, cs);

    make_rcs_descset_layout(rb->dev, &rcs_descset_layout, cs);

    make_rcs_red_descset_layout(rb->dev, &rcs_red_descset_layout, cs);

    make_sampler(rb, &rcs_sampler, cs);

    for (u32 i = 0; i < N_MAX_INFLIGHT; i++) {
        rcs_inflights[i] = (RcsPerInflight){};

        make_rcs_depthresources(rb, ext, &rcs_inflights[i].depthimg, cs);
        make_rcs_rendertargets(rb, ext, N_RENDTARGETS,
                               rcs_inflights[i].rendtargets, cs);
        make_rcs_ubo(rb, &rcs_inflights[i].ubo, cs);
        vmaMapMemory(rb->alloc, rcs_inflights[i].ubo.alloc,
                     &rcs_inflights[i].ubufmap);
        CLEANUP_START_NORES(MappingCleanup){
            .allocctx = rb->alloc,
            .alloc = rcs_inflights[i].ubo.alloc,
        } CLEANUP_END(mapping);

        make_rcs_fftbuf(rb, &rcs_inflights[i].fft_buf, cs);

        make_rcs_fftimg(rb, ext, &rcs_inflights[i].fft_img, cs);

        make_rcs_extrbuf(rb, &rcs_inflights[i].extr_buf, cs);

        make_rcs_descset(rb, rcs_dpool, rcs_descset_layout,
                         rcs_inflights[i].ubo, &rcs_inflights[i].descset);

        make_rcs_red_descset(rb, rcs_dpool, rcs_inflights[i].fft_img,
                             rcs_sampler, rcs_inflights[i].extr_buf,
                             rcs_red_descset_layout,
                             &rcs_inflights[i].red_descset);

        make_rcs_cmdbuf(rb, cpool, &rcs_inflights[i].cmdbuf, cs);
    }

    make_rcs_pipeline(rb, ext, rcs_descset_layout, col_formats, &depth_format,
                      &rcs_pipeline_layout, &rcs_pipeline, &e, cs);

    make_rcs_reduction_pipeline(rb, rcs_red_descset_layout,
                                &rcs_red_pipeline_layout, &rcs_red_pipeline,
                                cs);

    make_rcs_mesh(rb, cpool, &rcs_mesh.vertexbuf, &rcs_mesh.indexbuf,
                  &rcs_mesh.n_indices, cs);

    RcsResources res = {ext,
                        rcs_dpool,
                        rcs_descset_layout,
                        rcs_pipeline_layout,
                        rcs_pipeline,
                        rcs_red_pipeline_layout,
                        rcs_red_pipeline,
                        rcs_sampler,
                        rcs_mesh,
                        pathres,
                        };

    memcpy(((void*)&res) + offsetof(RcsResources, sets), rcs_inflights,
           sizeof(RcsPerInflight) * N_MAX_INFLIGHT);

    *out_res = res;
    return false;
}