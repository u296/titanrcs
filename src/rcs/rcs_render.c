#include "rcs/rcs_render.h"

#include "common.h"
#include "context.h"
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
    rpbi.pClearValues = (VkClearValue[]){clearcol, clearcol, clearcol, clear_depth};

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
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->rcs_resources.pipeline);

    vkCmdBindVertexBuffers(cmdbuf, 0, 1, vbufs, vbuf_offsets);
    vkCmdBindIndexBuffer(cmdbuf, ctx->rcs_resources.mesh.indexbuf.buf, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            ctx->rcs_resources.pipeline_layout, 0, 1, &ctx->rcs_resources.descset,
                            0, NULL);

    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

    vkCmdDrawIndexed(cmdbuf, 6, 1, 0, 0, 0);
    // vkCmdDraw(cmdbuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdbuf);

    

    // TEMPORARY HACK TO BLIT TO SWAPCHAIN

    VkImageMemoryBarrier renderBarrier = {};
    renderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    renderBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    renderBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    renderBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    renderBarrier.image = ctx->rcs_resources.rendtargets[0].img; // Your offscreen image
    renderBarrier.subresourceRange =
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &renderBarrier);

    // END HACK

    VkResult r = vkEndCommandBuffer(cmdbuf);
    assert(r == VK_SUCCESS);

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 0;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmdbuf;
    si.signalSemaphoreCount = 0;

    r = vkQueueSubmit(ctx->backend.queues.graphics_queue, 1, &si, VK_NULL_HANDLE);
    assert(r == VK_SUCCESS);

    vkQueueWaitIdle(ctx->backend.queues.graphics_queue);
}