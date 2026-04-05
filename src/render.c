#include "render.h"
#include "common.h"
#include "context.h"
#include "descriptors.h"
#include "linalg.h"
#include "rcs/rcs.h"
#include "rcs/rcs_render.h"
#include "rcs/rcs_ubo.h"
#include "res.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>

void write_interface_ubo(u64 frame, VkExtent2D swp_ext, void* ubufmap) {
    InterfaceUbo u = {};

    f32 I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    Mat4 imat;
    memcpy(&imat.v, I, sizeof(I));

    f32 t = (f32)frame * 1.0 / 150.0;

    f32 rotz[16] = {cosf(t), -sinf(t), 0, 0, sinf(t), cosf(t), 0, 0,
                    0,       0,        1, 0, 0,       0,       0, 1};
    (void)rotz;

    f32 roty[16] = {cosf(t), 0, -sinf(t), 0, 0, 1, 0, 0,
                    sinf(t), 0, cosf(t),  0, 0, 0, 0, 1};

    f32 transz[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, -2, 0, 0, 0, 1};

    Mat4 transmat;
    memcpy(&transmat.v, transz, sizeof(I));

    f32 zfar = 10.0f;
    f32 znear = 0.01f;
    f32 fov = 120.0f * (3.1415926535f / 180.0f);

    f32 as = (f32)swp_ext.width / (f32)swp_ext.height;

    f32 f = tanf(fov / 2);

    /*f32 perspective[16] = {
        1.0f / (tanf(fov/2)*as), 0, 0, 0,
        0, 1.0f / tanf(fov/2), 0, 0,
        0, 0, zfar/(zfar-znear), 1,
        0, 0, -(zfar*znear)/(zfar - znear), 0
    };*/

    f32 perspective[16] = {f / as,
                           0,
                           0,
                           0,
                           0,
                           -f,
                           0,
                           0,
                           0,
                           0,
                           zfar / (znear - zfar),
                           (znear * zfar) / (znear - zfar),
                           0,
                           0,
                           -1,
                           0};
    Mat4 perspectivemat;
    memcpy(&perspectivemat.v, perspective, sizeof(I));

    f32 mixing = 1; // powf(sinf(0.5*t), 2.0f);

    Mat4 mixed_translation =
        add_m4(muls_m4(1.0f - mixing, imat), muls_m4(mixing, transmat));
    Mat4 mixed_perspective =
        add_m4(muls_m4(1.0f - mixing, imat), muls_m4(mixing, perspectivemat));

    memcpy(&u.model, roty, sizeof(I));
    memcpy(&u.view, &mixed_translation, sizeof(I));
    memcpy(&u.proj, &mixed_perspective, sizeof(I));

    static f32 a = 0.0;
    a += 0.2f * (3.1415f / 180.0f);
    f32 zoom = 0.1; // 0.6f + 0.6f*sinf(a);

    u.model = transpose_m4(u.model);
    u.view = transpose_m4(u.view);
    u.proj = transpose_m4(u.proj);
    u.fzoom_ = (Vec4){zoom, 0.0, 0.0, 0.0};

    memcpy(ubufmap, &u, sizeof(u));
}

bool record_interface_cmdbuf(VkExtent2D swapchainextent, VkCommandBuffer cmdbuf,
                         VkPipelineLayout pipeline_layout, VkPipeline pipeline,
                         VkDescriptorSet desc_set, Renderable ren,
                          RenderContext* ctx,
                         const u32 swpch_img_i) {

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = 0;
    cbbi.pInheritanceInfo = NULL;

    VkClearValue clearcol = {};
    clearcol.color.float32[0] = 0.0f;
    clearcol.color.float32[1] = 0.0f;
    clearcol.color.float32[2] = 0.0f;
    clearcol.color.float32[3] = 1.0f;

    VkRenderingAttachmentInfo swp_attach = {};
    swp_attach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    swp_attach.clearValue = clearcol;
    swp_attach.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    swp_attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    swp_attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    swp_attach.imageView = ctx->swapchain.swpch_imgvs[swpch_img_i];

    VkRenderingInfo ri = {};
    ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &swp_attach;
    ri.layerCount = 1;
    ri.renderArea = (VkRect2D){{0, 0}, ctx->swapchain.swpch_ext};

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = swapchainextent.width;
    viewport.height = swapchainextent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.extent = swapchainextent;
    scissor.offset = (struct VkOffset2D){0, 0};

    VkBuffer vbufs[] = {ren.vertexbuf.buf};
    VkDeviceSize vbuf_offsets[] = {0};

    vkBeginCommandBuffer(cmdbuf, &cbbi);

    VkImageMemoryBarrier bar_pre = {};
    bar_pre.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar_pre.image = ctx->swapchain.swpch_imgs[swpch_img_i];
    bar_pre.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bar_pre.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bar_pre.srcAccessMask = VK_ACCESS_NONE;
    bar_pre.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    bar_pre.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bar_pre.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    bar_pre.subresourceRange =
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                         NULL, 0, NULL, 1, &bar_pre);

    // vkCmdBeginRenderPass(cmdbuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBeginRendering(cmdbuf, &ri);

    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindVertexBuffers(cmdbuf, 0, 1, vbufs, vbuf_offsets);
    vkCmdBindIndexBuffer(cmdbuf, ren.indexbuf.buf, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_layout, 0, 1, &desc_set, 0, NULL);

    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

    for (u32 i = 0; i < 4; i++) {
        // u32 tex_i = 3; // rand() % 4;
        vkCmdPushConstants(cmdbuf, pipeline_layout,
                           VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(u32), &i);

        vkCmdDrawIndexed(cmdbuf, 6, 1, i * 6, 0, 0);
        // vkCmdDraw(cmdbuf, 3, 1, 0, 0);
    }
    // vkCmdEndRenderPass(cmdbuf);
    vkCmdEndRendering(cmdbuf);

    VkImageMemoryBarrier bar_post = {};
    bar_post.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar_post.image = ctx->swapchain.swpch_imgs[swpch_img_i];
    bar_post.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bar_post.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bar_post.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    bar_post.dstAccessMask = VK_ACCESS_NONE;
    bar_post.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    bar_post.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    bar_post.subresourceRange =
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                         NULL, 1, &bar_post);

    const bool blittoscreenbuffer = false;

    // TEMPORARY HACK TO BLIT TO SCREENBUFFER
    if (blittoscreenbuffer) {
        VkImageMemoryBarrier swapBarrier = {};
        swapBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        swapBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        swapBarrier.srcAccessMask = 0;
        swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        swapBarrier.image = ctx->swapchain.swpch_imgs[swpch_img_i];
        swapBarrier.subresourceRange =
            (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0,
                             NULL, 1, &swapBarrier);

        VkImageBlit bi = {};
        bi.srcOffsets[0] = (VkOffset3D){0, 0, 0};
        bi.srcOffsets[1] = (VkOffset3D){RCS_RESOLUTION, RCS_RESOLUTION, 1};
        bi.srcSubresource =
            (VkImageSubresourceLayers){VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};

        i32 x = ctx->swapchain.swpch_ext.width;
        i32 y = ctx->swapchain.swpch_ext.height;

        bi.dstOffsets[0] = (VkOffset3D){0, 0, 0};
        bi.dstOffsets[1] = (VkOffset3D){x, y, 1};
        bi.dstSubresource =
            (VkImageSubresourceLayers){VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};

        vkCmdBlitImage(cmdbuf, ctx->rcs_resources.sets[0].fft_img.img,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       ctx->swapchain.swpch_imgs[swpch_img_i],
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bi,
                       VK_FILTER_NEAREST);

        VkImageMemoryBarrier presentBarrier = {};
        presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentBarrier.srcAccessMask =
            VK_ACCESS_TRANSFER_WRITE_BIT; // Wait for blit to finish
        presentBarrier.dstAccessMask =
            0; // Presentation doesn't need specific access
        presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentBarrier.image = ctx->swapchain.swpch_imgs[swpch_img_i];
        presentBarrier.subresourceRange =
            (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCmdPipelineBarrier(
            cmdbuf,
            VK_PIPELINE_STAGE_TRANSFER_BIT,       // From the blit stage
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // To the very end
            0, 0, nullptr, 0, nullptr, 1, &presentBarrier);
    }
    // END HACK

    VkResult r = vkEndCommandBuffer(cmdbuf);
    assert(r == VK_SUCCESS);

    return false;
}

LoopStatus renderloop_visualonly(RenderContext* ctx) {
    volatile VkResult f = VK_ERROR_UNKNOWN;
    f; // to silence unused variable warning in release builds
    struct Error e = {};

    bool before_first_render = true;

    while (!glfwWindowShouldClose(ctx->backend.wnd)) {
        glfwPollEvents();

        // vkDeviceWaitIdle(ctx->backend.dev);
        // usleep(1000*300);

        const u64 i_inflight =
            ctx->metadata.i_current_frame % ctx->resources.n_inflight_frames;

        f = vkWaitForFences(ctx->backend.dev, 1,
                            &ctx->resources.inflight_fncs[i_inflight], VK_TRUE,
                            UINT32_MAX);

        if (!before_first_render) {
            VmaAllocation a =
                ctx->rcs_resources.sets[i_inflight].extr_buf.alloc;

            vmaInvalidateAllocation(ctx->backend.alloc, a, 0,
                                    sizeof(ExtractionSsbo));
            VmaAllocationInfo info = {};
            vmaGetAllocationInfo(ctx->backend.alloc, a, &info);

            ExtractionSsbo* out = info.pMappedData;

            f32 rcs = out->out_rcs;

            if (ctx->metadata.i_current_frame % 1 == 0) {
                printf("RCS: %8.1f    ", rcs); // leave space for fps print
            }
        }

        u32 i_image = UINT32_MAX;
        VkResult img_ac_res = vkAcquireNextImageKHR(
            ctx->backend.dev, ctx->swapchain.swpch, UINT64_MAX,
            ctx->resources.img_ready_sems[i_inflight], VK_NULL_HANDLE,
            &i_image);

        switch (img_ac_res) {
        case VK_ERROR_OUT_OF_DATE_KHR:
            return REMAKE_SWAPCHAIN;
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_SUCCESS:
            break;
        default:
            goto fail;
            break;
        }

        f = vkResetFences(ctx->backend.dev, 1,
                          &ctx->resources.inflight_fncs[i_inflight]);
        f = vkResetCommandBuffer(ctx->resources.cmd_bufs[i_inflight], 0);

        write_interface_ubo(ctx->metadata.i_current_frame,
                            ctx->swapchain.swpch_ext,
                            ctx->resources.ubuf_mappings[i_inflight]);

        write_rcs_ubo(ctx, ctx->rcs_resources.sets[i_inflight].ubufmap);

        f = record_interface_cmdbuf(ctx->swapchain.swpch_ext,

                                ctx->resources.cmd_bufs[i_inflight],

                                ctx->framegraph.pipeline_layout,
                                ctx->framegraph.pipeline,
                                ctx->framegraph.desc_sets[i_inflight],
                                ctx->framegraph.the_object, ctx, i_image);

        VkCommandBuffer cmdbufs[2] = {
            ctx->rcs_resources.sets[i_inflight].cmdbuf,
            ctx->resources.cmd_bufs[i_inflight]};

        VkSemaphore waitsems[1] = {ctx->resources.img_ready_sems[i_inflight]};
        VkSemaphore render_signal_sems[1] = {
            ctx->resources.render_finished_sems[i_image]};
        VkPipelineStageFlags waitstages[1] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo si = {};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = waitsems;
        si.pWaitDstStageMask = waitstages;
        si.commandBufferCount = 2;
        si.pCommandBuffers = cmdbufs;
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = render_signal_sems;

        vkQueueSubmit(ctx->backend.queues.graphics_queue, 1, &si,
                      ctx->resources.inflight_fncs[i_inflight]);

        before_first_render = false;

        VkPresentInfoKHR pi = {};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = render_signal_sems;
        pi.swapchainCount = 1;
        pi.pSwapchains = &ctx->swapchain.swpch;
        pi.pImageIndices = &i_image;
        pi.pResults = NULL;

        VkResult pres_res =
            vkQueuePresentKHR(ctx->backend.queues.present_queue, &pi);

        (ctx->metadata.i_current_frame)++;

        if (ctx->metadata.i_current_frame %
                ctx->config.n_frameratecheck_interval ==
            (ctx->config.n_frameratecheck_interval - 1)) {
            struct timespec now = {};
            timespec_get(&now, TIME_UTC);
            struct timespec then = ctx->metadata.last_frame_time;

            f32 elapsed_time =
                (f32)(now.tv_sec - then.tv_sec) +
                (f32)(now.tv_nsec - then.tv_nsec) / 1000000000.0f;

            const float fps =
                (float)ctx->config.n_frameratecheck_interval / elapsed_time;

            ctx->metadata.last_frame_time = now;

            printf("FPS: %.1f\n", fps);
        } else {
            printf("\n");
        }

        switch (pres_res) {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            ctx->backend.fb_resized = false;
            return REMAKE_SWAPCHAIN;
            break;
        default:
            goto fail;
            break;
        }

        if (ctx->backend.fb_resized) {
            ctx->backend.fb_resized = false;
            return REMAKE_SWAPCHAIN;
        }
    }
fail:
    return EXIT_PROGRAM;
}
