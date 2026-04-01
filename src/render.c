#include "render.h"
#include "common.h"
#include "context.h"
#include "descriptors.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void update_uniformbuffer(u64 frame, VkExtent2D swp_ext, void* ubufmap) {
    UniformBufferObject u = {};

    f32 I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    Mat4 imat;
    memcpy(&imat.v, I, sizeof(I));

    f32 t = (f32)frame * 1.0 / 150.0;

    f32 rotz[16] = {cosf(t), -sinf(t), 0, 0, sinf(t), cosf(t), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    (void)rotz;

    f32 roty[16] = {cosf(t), 0, -sinf(t), 0, 0, 1, 0, 0, sinf(t), 0, cosf(t), 0, 0, 0, 0, 1};

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

    f32 perspective[16] = {
        f / as, 0, 0,  0, 0, -f, 0, 0, 0, 0, zfar / (znear - zfar), (znear * zfar) / (znear - zfar),
        0,      0, -1, 0};
    Mat4 perspectivemat;
    memcpy(&perspectivemat.v, perspective, sizeof(I));

    f32 mixing = 1; // powf(sinf(0.5*t), 2.0f);

    Mat4 mixed_translation = add_m4(muls_m4(1.0f - mixing, imat), muls_m4(mixing, transmat));
    Mat4 mixed_perspective = add_m4(muls_m4(1.0f - mixing, imat), muls_m4(mixing, perspectivemat));

    memcpy(&u.model, roty, sizeof(I));
    memcpy(&u.view, &mixed_translation, sizeof(I));
    memcpy(&u.proj, &mixed_perspective, sizeof(I));

    u.model = transpose_m4(u.model);
    u.view = transpose_m4(u.view);
    u.proj = transpose_m4(u.proj);

    memcpy(ubufmap, &u, sizeof(u));
}

bool recordcommandbuffer(VkExtent2D swapchainextent, VkFramebuffer fb, VkCommandBuffer cmdbuf,
                         VkRenderPass renderpass, VkPipelineLayout pipeline_layout,
                         VkPipeline pipeline, VkDescriptorSet desc_set, Renderable ren,
                         struct Error* e_out, RenderContext* ctx, const u32 swpch_img_i) {

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = 0;
    cbbi.pInheritanceInfo = NULL;

    VkRenderPassBeginInfo rpbi = {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = renderpass;
    rpbi.framebuffer = fb;
    rpbi.renderArea.offset = (struct VkOffset2D){0, 0};
    rpbi.renderArea.extent = swapchainextent;

    VkClearValue clearcol = {};
    clearcol.color.float32[0] = 0.0f;
    clearcol.color.float32[1] = 0.0f;
    clearcol.color.float32[2] = 0.0f;
    clearcol.color.float32[3] = 1.0f;

    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &clearcol;

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

    vkCmdBeginRenderPass(cmdbuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindVertexBuffers(cmdbuf, 0, 1, vbufs, vbuf_offsets);
    vkCmdBindIndexBuffer(cmdbuf, ren.indexbuf.buf, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &desc_set, 0, NULL);

    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

    vkCmdDrawIndexed(cmdbuf, 6, 1, 0, 0, 0);
    // vkCmdDraw(cmdbuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdbuf);

    // TEMPORARY HACK TO BLIT TO SCREENBUFFER

    VkImageMemoryBarrier swapBarrier = {};
    swapBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapBarrier.srcAccessMask = 0;
    swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    swapBarrier.image = ctx->swapchain.swpch_imgs[swpch_img_i];
    swapBarrier.subresourceRange = (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &swapBarrier);

    VkImageBlit bi = {};
    bi.srcOffsets[0] = (VkOffset3D){0, 0, 0};
    bi.srcOffsets[1] = (VkOffset3D){256, 256, 1};
    bi.srcSubresource = (VkImageSubresourceLayers){VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};

    i32 x = ctx->swapchain.swpch_ext.width;
    i32 y = ctx->swapchain.swpch_ext.height;

    bi.dstOffsets[0] = (VkOffset3D){0, 0, 0};
    bi.dstOffsets[1] = (VkOffset3D){x, y, 1};
    bi.dstSubresource = (VkImageSubresourceLayers){VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};

    vkCmdBlitImage(cmdbuf, ctx->rcs_resources.fft_img.img,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ctx->swapchain.swpch_imgs[swpch_img_i],
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bi, VK_FILTER_NEAREST);

    VkImageMemoryBarrier presentBarrier = {};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Wait for blit to finish
    presentBarrier.dstAccessMask = 0; // Presentation doesn't need specific access
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.image = ctx->swapchain.swpch_imgs[swpch_img_i];
    presentBarrier.subresourceRange =
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(cmdbuf,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,       // From the blit stage
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // To the very end
                         0, 0, nullptr, 0, nullptr, 1, &presentBarrier);
    // END HACK

    VkResult r = vkEndCommandBuffer(cmdbuf);
    VERIFY("recording", r)

    return false;
}

LoopStatus do_renderloop(RenderContext* ctx) {
    volatile VkResult f = VK_ERROR_UNKNOWN;
    f; // to silence unused variable warning in release builds
    struct Error e = {};

    while (!glfwWindowShouldClose(ctx->backend.wnd)) {
        glfwPollEvents();

        const u64 i_frame_modn = ctx->metadata.i_current_frame % ctx->resources.n_inflight_frames;

        f = vkWaitForFences(ctx->backend.dev, 1, &ctx->resources.inflight_fncs[i_frame_modn],
                            VK_TRUE, UINT32_MAX);

        u32 i_image = UINT32_MAX;
        VkResult img_ac_res = vkAcquireNextImageKHR(
            ctx->backend.dev, ctx->swapchain.swpch, UINT64_MAX,
            ctx->resources.img_ready_sems[i_frame_modn], VK_NULL_HANDLE, &i_image);

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

        f = vkResetFences(ctx->backend.dev, 1, &ctx->resources.inflight_fncs[i_frame_modn]);
        f = vkResetCommandBuffer(ctx->resources.cmd_bufs[i_frame_modn], 0);

        update_uniformbuffer(ctx->metadata.i_current_frame, ctx->swapchain.swpch_ext,
                             ctx->resources.ubuf_mappings[i_frame_modn]);

        f = recordcommandbuffer(ctx->swapchain.swpch_ext, ctx->swapchain.fbufs[i_image],
                                ctx->resources.cmd_bufs[i_frame_modn], ctx->swapchain.renderpass,
                                ctx->framegraph.pipeline_layout, ctx->framegraph.pipeline,
                                ctx->framegraph.desc_sets[i_frame_modn], ctx->framegraph.the_object,
                                &e, ctx, i_image);

        VkSemaphore waitsems[1] = {ctx->resources.img_ready_sems[i_frame_modn]};
        VkSemaphore render_signal_sems[1] = {ctx->resources.render_finished_sems[i_image]};
        VkPipelineStageFlags waitstages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo si = {};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = waitsems;
        si.pWaitDstStageMask = waitstages;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &ctx->resources.cmd_bufs[i_frame_modn];
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = render_signal_sems;

        vkQueueSubmit(ctx->backend.queues.graphics_queue, 1, &si,
                      ctx->resources.inflight_fncs[i_frame_modn]);

        VkPresentInfoKHR pi = {};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = render_signal_sems;
        pi.swapchainCount = 1;
        pi.pSwapchains = &ctx->swapchain.swpch;
        pi.pImageIndices = &i_image;
        pi.pResults = NULL;

        VkResult pres_res = vkQueuePresentKHR(ctx->backend.queues.present_queue, &pi);

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

        (ctx->metadata.i_current_frame)++;

        if (ctx->metadata.i_current_frame % ctx->config.n_frameratecheck_interval ==
            ctx->config.n_frameratecheck_interval - 1) {
            const clock_t time_now = clock();

            const float fps = (float)ctx->config.n_frameratecheck_interval * (float)CLOCKS_PER_SEC /
                              (float)(time_now - ctx->metadata.last_frame_time);

            ctx->metadata.last_frame_time = time_now;

            printf("FPS: %.0f\n", fps);
        }
    }
fail:
    return EXIT_PROGRAM;
}
