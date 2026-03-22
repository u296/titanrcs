#include "rcs/rcs_populate.h"

#include "context.h"


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

    rpbi.clearValueCount = 4;
    rpbi.pClearValues = (VkClearValue[]){clearcol, clearcol, clearcol, clearcol};

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

    VkBuffer vbufs[] = {ren.vertexbuf.buf};
    VkDeviceSize vbuf_offsets[] = {0};

    VkCommandBuffer cmdbuf = ctx->resources.cmd_bufs[0]; // should be alright

    vkBeginCommandBuffer(cmdbuf, &cbbi);

    vkCmdBeginRenderPass(cmdbuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->rcs_resources.pipeline);

    vkCmdBindVertexBuffers(cmdbuf, 0, 1, vbufs, vbuf_offsets);
    vkCmdBindIndexBuffer(cmdbuf, ren.indexbuf.buf, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &desc_set, 0, NULL);

    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

    vkCmdDrawIndexed(cmdbuf, 6, 1, 0, 0, 0);
    // vkCmdDraw(cmdbuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdbuf);

    VkResult r = vkEndCommandBuffer(cmdbuf);


}