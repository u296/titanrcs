
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <volk.h>

#include "cleanupstack.h"
#include "common.h"
#include "descriptors.h"

#include "buffers.h"
#include "pipeline.h"
#include "render.h"
#include "resources/renderresources.h"
#include "rcs/rcs.h"

#include "vulkan/vulkan_core.h" // having this here doesn't hurt and  prevents intellisense from adding it at the top which would break compilation

#define MAINCHECK                                                                                  \
    if (f) {                                                                                       \
        printf("Error, code=%d: %s\n", e.code, e.origin);                                          \
        goto fail;                                                                                 \
    }

#include "context.h"

int main() {

    RenderContext ctx = {};

    struct CleanupStack cs = {};
    cs_init(&cs);

    volkInitialize();
    bool f = false;

    struct Error e = {.code = 0, .origin = ""};

    VkDescriptorSetLayout my_desc_set_layout;
    Renderable tri;
    VkDescriptorPool my_dpool;

    init_backend(&ctx.backend, &cs);

    CleanupStack swp_cs = {};
    cs_init(&swp_cs);

    make_swapchain_context(ctx.backend, &ctx.swapchain, &swp_cs);

    make_renderresources(&ctx, &cs);

    make_rcs_setup(&ctx.backend, &cs);

    f = make_descriptorsetlayout(ctx.backend.dev, &my_desc_set_layout, &cs);
    MAINCHECK

    f = make_graphicspipeline(ctx.backend.dev, ctx.swapchain.swpch_ext, ctx.swapchain.renderpass,
                              my_desc_set_layout, &ctx.framegraph.pipeline_layout,
                              &ctx.framegraph.pipeline, &e, &cs);
    MAINCHECK

    f = make_vertexbuffer(ctx.backend.physdev, ctx.backend.dev, ctx.backend.queues,
                          ctx.resources.cmd_pool, &tri.vertexbuf, &e, &cs);
    MAINCHECK

    f = make_indexbuffer(ctx.backend.physdev, ctx.backend.dev, ctx.backend.queues,
                         ctx.resources.cmd_pool, &tri.indexbuf, &e, &cs);
    MAINCHECK

    f = make_descriptor_pool(ctx.resources.n_inflight_frames, ctx.backend.dev, &my_dpool, &e, &cs);
    MAINCHECK

    f = make_descriptorsetlayout(ctx.backend.dev, &my_desc_set_layout, &cs);
    MAINCHECK

    f = make_descriptor_sets(ctx.resources.n_inflight_frames, ctx.backend.dev, my_dpool,
                             ctx.resources.ubufs, my_desc_set_layout, &ctx.framegraph.desc_sets, &e,
                             &cs);
    MAINCHECK

    // u64 i_frame = 0;
    constexpr u64 n_frameratecheck = 100;

    clock_t last_time = clock();

    bool firstiter = true;

    bool shouldclose = false;

    // make rendercontext here
    /*
            RenderContext ctx = {
                    .metadata={
                            0,
                            last_time,
                    },
                    .config={
                            n_max_inflight,
                            n_frameratecheck
                    },
                    .backend = my_rendbackend,
                    .swapchain={
                            my_swapchain,
                            swapchain_extent,
                            n_swapchain_images,
                            swapchain_images,
                            my_imageviews,
                            my_framebuffers
                    },
                    .resources={
                            my_commandbufs,
                            sem_imgready,
                            sem_rendfinish,
                            fen_inflight,
                            my_ubuf_mappings
                    },
                    .framegraph={
                            tri.pipeline,
                            my_renderpass,
                            my_desc_sets,
                            tri
                    }
            };*/

    ctx.metadata.i_current_frame = 0;
    ctx.metadata.last_frame_time = last_time;
    // ctx.config.max_inflight_frames = n_max_inflight;
    ctx.config.n_frameratecheck_interval = n_frameratecheck;

    // ctx.backend = my_rendbackend;
    // ctx.swapchain = my_swpctx;
    // ctx.framegraph.pipeline=tri.pipeline;
    ctx.framegraph.the_object = tri;

    do {
        // this is the loop that owns the swapchain and recreates it whenever the renderloop exits
        // because the swapchain needs renewal

        if (!firstiter) {

            int width = 0, height = 0;

            glfwGetFramebufferSize(ctx.backend.wnd, &width, &height);
            while (width == 0 || height == 0) {
                glfwGetFramebufferSize(ctx.backend.wnd, &width, &height);
                glfwWaitEvents();
            }

            // if we get here it means the swapchain needs to be recreated
            cs_init(&swp_cs);

            make_swapchain_context(ctx.backend, &ctx.swapchain, &swp_cs);
        }

        LoopStatus l = do_renderloop(&ctx);

        cs_consume(&swp_cs);

        switch (l) {
        case REMAKE_SWAPCHAIN:
            break;
        case EXIT_PROGRAM:
            shouldclose = true;
            break;
        }

        firstiter = false;
    } while (!shouldclose);

    vkDeviceWaitIdle(ctx.backend.dev);

    cs_consume(&cs);
    return 0;
fail:
    cs_consume(&swp_cs);
    cs_consume(&cs);
    return 1;
}