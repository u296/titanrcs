#include "rcs/rcs_render.h"

#include "common.h"
#include "context.h"
#include "vkFFT/vkFFT_AppManagement/vkFFT_RunApp.h"
#include "vkFFT/vkFFT_Structs/vkFFT_Structs.h"
#include <assert.h>

void render_rcs_imgs(RenderContext* ctx) {

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = 0;
    cbbi.pInheritanceInfo = NULL;

    VkRenderPassBeginInfo rpbi = {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = ctx->rcs_resources.renderpass;
    rpbi.framebuffer = ctx->rcs_resources.framebuffer;
    rpbi.renderArea.offset = (struct VkOffset2D){0, 0};
    rpbi.renderArea.extent = ctx->rcs_resources.ext;

    VkClearValue clearcol = {};
    clearcol.color.float32[0] = 0.0f;
    clearcol.color.float32[1] = 0.0f;
    clearcol.color.float32[2] = 0.0f;
    clearcol.color.float32[3] = 1.0f;

    VkClearValue clear_depth = {};
    clear_depth.depthStencil.depth = 1.0;
    clear_depth.depthStencil.stencil = 0;

    rpbi.clearValueCount = 4;
    rpbi.pClearValues =
        (VkClearValue[]){clearcol, clearcol, clearcol, clear_depth};

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = ctx->rcs_resources.ext.width;
    viewport.height = ctx->rcs_resources.ext.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.extent = ctx->rcs_resources.ext;
    scissor.offset = (struct VkOffset2D){0, 0};

    VkBuffer vbufs[] = {ctx->rcs_resources.mesh.vertexbuf.buf};
    VkDeviceSize vbuf_offsets[] = {0};

    VkCommandBuffer cmdbuf = ctx->resources.cmd_bufs[0]; // should be alright

    vkBeginCommandBuffer(cmdbuf, &cbbi);

    vkCmdBeginRenderPass(cmdbuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      ctx->rcs_resources.pipeline);

    vkCmdBindVertexBuffers(cmdbuf, 0, 1, vbufs, vbuf_offsets);
    vkCmdBindIndexBuffer(cmdbuf, ctx->rcs_resources.mesh.indexbuf.buf, 0,
                         VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            ctx->rcs_resources.pipeline_layout, 0, 1,
                            &ctx->rcs_resources.descset, 0, NULL);

    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

    vkCmdDrawIndexed(cmdbuf, 6, 1, 0, 0, 0);
    // vkCmdDraw(cmdbuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdbuf);

    // TEMPORARY HACK TO BLIT TO SWAPCHAIN
    // also allows for transfer to the fft buffer

    VkImageMemoryBarrier renderBarrier = {};
    renderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    renderBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    renderBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    renderBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    renderBarrier.image =
        ctx->rcs_resources.rendtargets[0].img; // Your offscreen image
    renderBarrier.subresourceRange =
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                         &renderBarrier);

    // END HACK

    VkBuffer fft_buf = ctx->rcs_resources.fft_buf.buf;

    // the buffer copies can be modified to do the layout shifting to center the
    // fft

    VkBufferImageCopy reg = {};
    reg.bufferOffset = 0;
    reg.bufferRowLength = 0;
    reg.bufferImageHeight = 0;

    reg.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    reg.imageSubresource.mipLevel = 0;
    reg.imageSubresource.baseArrayLayer = 0;
    reg.imageSubresource.layerCount = 1;

    const u32 w = 256, h = 256; // should change this to a
                                // more global solution

    reg.imageOffset = (VkOffset3D){0, 0, 0};
    reg.imageExtent = (VkExtent3D){w, h, 1};

    vkCmdCopyImageToBuffer(cmdbuf, ctx->rcs_resources.rendtargets[0].img,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, fft_buf, 1,
                           &reg);

    VkBufferMemoryBarrier bar_bufpostcp = {};
    bar_bufpostcp.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bar_bufpostcp.buffer = fft_buf;
    bar_bufpostcp.offset = 0;
    bar_bufpostcp.size = VK_WHOLE_SIZE;
    bar_bufpostcp.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bar_bufpostcp.dstAccessMask =
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
    bar_bufpostcp.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bar_bufpostcp.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 1,
                         &bar_bufpostcp, 0, NULL);

    VkFFTLaunchParams lp = {};
    lp.commandBuffer = &cmdbuf;
    lp.buffer = &fft_buf;

    VkFFTAppend(ctx->backend.fft, -1, &lp);

    VkBufferMemoryBarrier bar_bufpostfft = {};
    bar_bufpostfft.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bar_bufpostfft.buffer = fft_buf;
    bar_bufpostfft.offset = 0;
    bar_bufpostfft.size = VK_WHOLE_SIZE;
    bar_bufpostfft.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    bar_bufpostfft.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    bar_bufpostfft.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bar_bufpostfft.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    VkImageMemoryBarrier bar_imgpostfft = {};
    bar_imgpostfft.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar_imgpostfft.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    bar_imgpostfft.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bar_imgpostfft.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    bar_imgpostfft.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bar_imgpostfft.image =
        ctx->rcs_resources.rendtargets[0].img; // Your offscreen image
    bar_imgpostfft.subresourceRange =
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkImageMemoryBarrier bar_fftimg_postfft = {};
    bar_fftimg_postfft.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar_fftimg_postfft.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bar_fftimg_postfft.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    bar_fftimg_postfft.srcAccessMask = VK_ACCESS_NONE;
    bar_fftimg_postfft.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bar_fftimg_postfft.image =
        ctx->rcs_resources.fft_img.img; // Your offscreen image
    bar_fftimg_postfft.subresourceRange =
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    const VkImageMemoryBarrier postfft_imgbars[2] = {bar_imgpostfft, bar_fftimg_postfft};

    vkCmdPipelineBarrier(
        cmdbuf,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, NULL, 1, &bar_bufpostfft, 2, postfft_imgbars
        );

    vkCmdCopyBufferToImage(cmdbuf, fft_buf, ctx->rcs_resources.fft_img.img,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &reg);

    VkImageMemoryBarrier bar_imgpostfftcopy = {};
    bar_imgpostfftcopy.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar_imgpostfftcopy.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    bar_imgpostfftcopy.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    bar_imgpostfftcopy.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bar_imgpostfftcopy.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    bar_imgpostfftcopy.image = ctx->rcs_resources.fft_img.img;
    bar_imgpostfftcopy.subresourceRange =
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                         &bar_imgpostfftcopy);

    VkResult r = vkEndCommandBuffer(cmdbuf);
    assert(r == VK_SUCCESS);

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 0;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmdbuf;
    si.signalSemaphoreCount = 0;

    r = vkQueueSubmit(ctx->backend.queues.graphics_queue, 1, &si,
                      VK_NULL_HANDLE);
    assert(r == VK_SUCCESS);

    vkQueueWaitIdle(ctx->backend.queues.graphics_queue);
}