#include "GLFW/glfw3.h"
#include "cleanupstack.h"
#include "common.h"
#include "context.h"
#include "descriptors.h"
#include "interface/int_render.h"
#include "rcs/rcs.h"
#include "rcs/rcs_pathing.h"
#include "rcs/rcs_ubo.h"
#include "render.h"
#include "res.h"
#include <assert.h>
#include <stdio.h>
#include <Python.h>
#include <unistd.h>

#define PI (3.1415926535f)
#define RADIANS (3.1415926535f / 180.0f)


f32 calc_rcs(f32 s_at_object, f32 s_at_observer, f32 dist) {
    return s_at_observer * 4.0f * PI * dist * dist / s_at_object;
}


void extract_and_write(FILE* output, void* extr_map, PathingResources* pres, Path* path) {
    ExtractionSsbo* extr = extr_map;

    path_write_statcols(pres,path, output);

    f32 rcs = calc_rcs(1.0, extr->out_intensity, 1000.0);

    fprintf(output, "%f\n", rcs);
}

void run_computepass(RenderContext* ctx) {
    FILE* outputfile = NULL;

    outputfile = fopen("computepass.csv", "w");

    fprintf(outputfile, "hello\n");

    VmaAllocation extraction_allocs[N_MAX_INFLIGHT];
    void* extraction_maps[N_MAX_INFLIGHT];
    bool first_render_stats[N_MAX_INFLIGHT] = {};

    for (u32 i = 0; i < N_MAX_INFLIGHT; i++) {
        first_render_stats[i] = true;
        VmaAllocationInfo info = {};
        extraction_allocs[i] = ctx->rcs_resources.sets[i].extr_buf.alloc;

        vmaGetAllocationInfo(ctx->backend.alloc, extraction_allocs[i], &info);

        extraction_maps[i] = info.pMappedData;
    }

    Path* mypath = malloc(sizeof(Path));

    path_init(&ctx->rcs_resources.pathres,mypath);

    struct timespec start;
    timespec_get(&start, TIME_UTC);

    volatile VkResult r = VK_RESULT_MAX_ENUM;
    r;

    u32 i = 0;
    for (; !path_is_complete(&ctx->rcs_resources.pathres, mypath); i++) {

        const u32 f = i % N_MAX_INFLIGHT;

        RenderBackend* rb = &ctx->backend;
        void* ubo_mapping = ctx->rcs_resources.sets[f].ubufmap;
        VmaAllocation extraction_alloc = extraction_allocs[f];
        void* extraction_mapping = extraction_maps[f];
        bool* first_render = &first_render_stats[f];
        VkFence inflight_fen = ctx->resources.inflight_fncs[f];
        VkCommandBuffer cmdbuf = ctx->rcs_resources.sets[f].cmdbuf;

        path_write_ubo(&ctx->rcs_resources.pathres, mypath, ctx->rcs_resources.sets[f].ubufmap);

        r = vkWaitForFences(rb->dev, 1, &inflight_fen, VK_TRUE, UINT64_MAX);
        assert(r == VK_SUCCESS);
        if (!*first_render) {
            vmaInvalidateAllocation(rb->alloc, extraction_alloc, 0,
                                    sizeof(ExtractionSsbo));
            extract_and_write(outputfile, extraction_mapping, &ctx->rcs_resources.pathres, mypath);
        }

        path_write_ubo(&ctx->rcs_resources.pathres, mypath, ubo_mapping);

        r = vkResetFences(rb->dev, 1, &inflight_fen);
        assert(r == VK_SUCCESS);

        VkSubmitInfo sub = {};
        sub.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        sub.commandBufferCount = 1;
        sub.pCommandBuffers = &cmdbuf;
        // sub.;

        r = vkQueueSubmit(rb->queues.graphics_queue, 1, &sub, inflight_fen);
        assert(r == VK_SUCCESS);
        *first_render = false;

        path_advance(&ctx->rcs_resources.pathres, mypath);
    }

    struct timespec end;
    timespec_get(&end, TIME_UTC);

    f32 runtime = (f32)(end.tv_sec - start.tv_sec) +
                  ((f32)(end.tv_nsec - start.tv_nsec) / 1000000000.0f);

    f32 fps = (f32)i / runtime;

    printf("Ran %u renders in %.1f seconds, average FPS: %.1f\n", i, runtime,
           fps);

    fclose(outputfile);

    free(mypath);
}

typedef enum CompLoopStatus {
    COMP_REMAKE_SWAPCHAIN = 1,
    COMP_FAILURE = 2,
    COMP_COMPLETE = 3,
    COMP_WINDOW_CLOSED = 4,
} CompLoopStatus;

CompLoopStatus visual_compute_mainloop(RenderContext* ctx, FILE* outputfile,
                                       Path* master_path, Path* slave_paths,
                                       u32* i, VmaAllocation* extraction_allocs,
                                       void** extraction_maps,
                                       bool* first_render_stats) {
    VkResult r = VK_RESULT_MAX_ENUM;

    for (; !path_is_complete(&ctx->rcs_resources.pathres, master_path); (*i)++) {
        glfwPollEvents();
        if (glfwWindowShouldClose(ctx->backend.wnd)) {
            return COMP_WINDOW_CLOSED;
        }

        //usleep(1000*300);

        const u32 f = *i % N_MAX_INFLIGHT;

        RenderBackend* rb = &ctx->backend;
        Path* slave_path = &slave_paths[f];
        void* ubo_mapping = ctx->rcs_resources.sets[f].ubufmap;
        VmaAllocation extraction_alloc = extraction_allocs[f];
        void* extraction_mapping = extraction_maps[f];
        bool* first_render = &first_render_stats[f];
        VkFence inflight_fen = ctx->resources.inflight_fncs[f];
        VkCommandBuffer rcs_cmdbuf = ctx->rcs_resources.sets[f].cmdbuf;
        VkCommandBuffer int_cmdbuf = ctx->resources.cmd_bufs[f];

        r = vkWaitForFences(rb->dev, 1, &inflight_fen, VK_TRUE, UINT64_MAX);
        assert(r == VK_SUCCESS);
        if (!*first_render) {
            vmaInvalidateAllocation(rb->alloc, extraction_alloc, 0,
                                    sizeof(ExtractionSsbo));
            extract_and_write(outputfile, extraction_mapping,&ctx->rcs_resources.pathres, slave_path);
        }

        r = vkResetFences(rb->dev, 1, &inflight_fen);
        assert(r == VK_SUCCESS);

        u32 i_image = UINT32_MAX;
        VkResult img_ac_res = vkAcquireNextImageKHR(
            ctx->backend.dev, ctx->swapchain.swpch, UINT64_MAX,
            ctx->resources.img_ready_sems[f], VK_NULL_HANDLE, &i_image);

        switch (img_ac_res) {
        case VK_ERROR_OUT_OF_DATE_KHR:
            return COMP_REMAKE_SWAPCHAIN;
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_SUCCESS:
            break;
        default:
            return COMP_FAILURE;
            break;
        }

        r = vkResetCommandBuffer(ctx->resources.cmd_bufs[f], 0);

        write_interface_ubo(ctx->metadata.i_current_frame,
                            ctx->swapchain.swpch_ext,
                            ctx->resources.ubuf_mappings[f],
                        ctx->config.zoom);

        path_write_ubo(&ctx->rcs_resources.pathres, master_path, ubo_mapping);

        //*slave_path = *master_path;
        path_copy(slave_path, master_path);

        r = record_interface_cmdbuf(ctx->swapchain.swpch_ext,

                                    ctx->resources.cmd_bufs[f],

                                    ctx->framegraph.pipeline_layout,
                                    ctx->framegraph.pipeline,
                                    ctx->framegraph.desc_sets[f],
                                    ctx->framegraph.the_object, ctx, i_image);

        VkCommandBuffer cmdbufs[2] = {rcs_cmdbuf, int_cmdbuf};

        VkSemaphore waitsems[1] = {ctx->resources.img_ready_sems[f]};
        VkSemaphore render_signal_sems[1] = {
            ctx->resources.render_finished_sems[i_image]};
        VkPipelineStageFlags waitstages[1] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submit = {};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = waitsems;
        submit.pWaitDstStageMask = waitstages;
        submit.commandBufferCount = 2;
        submit.pCommandBuffers = cmdbufs;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = render_signal_sems;

        r = vkQueueSubmit(rb->queues.graphics_queue, 1, &submit, inflight_fen);
        assert(r == VK_SUCCESS);
        *first_render = false;

        VkPresentInfoKHR pres = {};

        pres.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pres.waitSemaphoreCount = 1;
        pres.pWaitSemaphores = render_signal_sems;
        pres.swapchainCount = 1;
        pres.pSwapchains = &ctx->swapchain.swpch;
        pres.pImageIndices = &i_image;
        pres.pResults = NULL;

        VkResult present_res =
            vkQueuePresentKHR(rb->queues.present_queue, &pres);

        switch (present_res) {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            ctx->backend.fb_resized = false;
            return COMP_REMAKE_SWAPCHAIN;
            break;
        default:
            return COMP_FAILURE;
            break;
        }

        if (ctx->backend.fb_resized) {
            ctx->backend.fb_resized = false;
            return COMP_REMAKE_SWAPCHAIN;
        }

        path_advance(&ctx->rcs_resources.pathres, master_path);
    }
    return COMP_COMPLETE;
}

void run_visual_computepass(RenderContext* ctx, CleanupStack* swp_cs) {
    


    FILE* outputfile = NULL;

    outputfile = fopen("computepass.csv", "w");

    fprintf(outputfile, "hello\n");

    VmaAllocation extraction_allocs[N_MAX_INFLIGHT];
    void* extraction_maps[N_MAX_INFLIGHT];
    bool first_render_stats[N_MAX_INFLIGHT] = {};

    for (u32 i = 0; i < N_MAX_INFLIGHT; i++) {
        first_render_stats[i] = true;
        VmaAllocationInfo info = {};
        extraction_allocs[i] = ctx->rcs_resources.sets[i].extr_buf.alloc;

        vmaGetAllocationInfo(ctx->backend.alloc, extraction_allocs[i], &info);

        extraction_maps[i] = info.pMappedData;
    }

    Path mypath;

    Path slave_paths[N_MAX_INFLIGHT];

    for (u32 i = 0; i < N_MAX_INFLIGHT; i++) {
        slave_paths[i] = (Path){};
    }

    path_init(&ctx->rcs_resources.pathres, &mypath);

    struct timespec start;
    timespec_get(&start, TIME_UTC);

    volatile VkResult r = VK_RESULT_MAX_ENUM;
    r;

    u32 i = 0;

    bool premature_exit = false;

    while (!path_is_complete(&ctx->rcs_resources.pathres, &mypath)) {
        CompLoopStatus status = visual_compute_mainloop(
            ctx, outputfile, &mypath, slave_paths, &i, extraction_allocs,
            extraction_maps, first_render_stats);

        switch (status) {
        case COMP_REMAKE_SWAPCHAIN:
            vkDeviceWaitIdle(ctx->backend.dev);

            cs_consume(swp_cs);
            {
                int width = 0, height = 0;

                glfwGetFramebufferSize(ctx->backend.wnd, &width, &height);
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize(ctx->backend.wnd, &width, &height);
                    glfwWaitEvents();
                }

                // if we get here it means the swapchain needs to be recreated
                cs_init(swp_cs);

                make_swapchain_context(ctx->backend, &ctx->swapchain, swp_cs);
            }
            break;
        case COMP_FAILURE: {
            printf("COMP FAILURE\n");
            abort();
        }
        case COMP_COMPLETE: {
            break;
        }
        case COMP_WINDOW_CLOSED: {
            premature_exit = true;
        }
        }
        if (premature_exit) {
            printf("Window closed: exiting early\n");
            break;
        }
    }

    struct timespec end;
    timespec_get(&end, TIME_UTC);

    f32 runtime = (f32)(end.tv_sec - start.tv_sec) +
                  ((f32)(end.tv_nsec - start.tv_nsec) / 1000000000.0f);

    f32 fps = (f32)i / runtime;

    printf("Ran %u renders in %.1f seconds, average FPS: %.1f\n", i, runtime,
           fps);

    fclose(outputfile);
}