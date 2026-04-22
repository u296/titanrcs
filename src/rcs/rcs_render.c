#include "rcs/rcs_render.h"

#include "common.h"
#include "context.h"
#include "linalg.h"
#include "rcs/rcs_ubo.h"
#include "res.h"
#include <assert.h>

// f is the inflight index
void record_rcs_cmdbuf(RenderContext* ctx, u32 f) {

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = 0;
    cbbi.pInheritanceInfo = NULL;

    VkClearValue prefft_clearcol = {};
    prefft_clearcol.color.float32[0] = 0.0f;
    prefft_clearcol.color.float32[0] = 0.0f;
    prefft_clearcol.color.float32[0] = 0.0f;
    prefft_clearcol.color.float32[0] = 0.0f;

    VkClearValue default_clearcol = {};
    default_clearcol.color.float32[0] = 0.0f;
    default_clearcol.color.float32[1] = 0.0f;
    default_clearcol.color.float32[2] = 0.0f;
    default_clearcol.color.float32[3] = 1.0f;

    VkClearValue clear_depth = {};
    clear_depth.depthStencil.depth = 1.0;
    clear_depth.depthStencil.stencil = 0;

    VkRenderingAttachmentInfo d = {};
    d.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    d.clearValue = clear_depth;
    d.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    d.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    d.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    d.imageView = ctx->rcs_resources.sets[f].depthimg.view;

    VkRenderingAttachmentInfo c[3] = {{}, {}, {}};

    for (u32 i = 0; i < 3; i++) {
        c[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        c[i].clearValue = default_clearcol;
        c[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        c[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        c[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        c[i].imageView = ctx->rcs_resources.sets[f].rendtargets[i].view;
    }

    c[0].clearValue = prefft_clearcol;

    VkRenderingInfo ri = {};
    ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    ri.layerCount = 1;
    ri.pDepthAttachment = &d;
    ri.colorAttachmentCount = 3;
    ri.pColorAttachments = c;
    ri.renderArea = (VkRect2D){{0, 0}, {RCS_RESOLUTION, RCS_RESOLUTION}};

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

    VkCommandBuffer cmdbuf = ctx->rcs_resources.sets[f].cmdbuf;

    vkBeginCommandBuffer(cmdbuf, &cbbi);

    VkImageMemoryBarrier2 rsb2[4];

    for (u32 i = 0; i < 4; i++) {
        rsb2[i] = (VkImageMemoryBarrier2){
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            NULL,
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,

        };
    }
    for (u32 i = 0; i < 3; i++) {
        rsb2[i].image = ctx->rcs_resources.sets[f].rendtargets[i].img;
        rsb2[i].subresourceRange =
            (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    }

    rsb2[3].dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    rsb2[3].dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    rsb2[3].newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    rsb2[3].image = ctx->rcs_resources.sets[f].depthimg.img;
    rsb2[3].subresourceRange =
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

    VkDependencyInfo prerenddep = {
        VK_STRUCTURE_TYPE_DEPENDENCY_INFO, NULL, 0, 0, NULL, 0, NULL, 4, rsb2};

    vkCmdPipelineBarrier2(cmdbuf, &prerenddep);

    vkCmdBeginRendering(cmdbuf, &ri);

    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      ctx->rcs_resources.po_pipeline);

    vkCmdBindVertexBuffers(cmdbuf, 0, 1, vbufs, vbuf_offsets);
    vkCmdBindIndexBuffer(cmdbuf, ctx->rcs_resources.mesh.indexbuf.buf, 0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            ctx->rcs_resources.rcs_pipline_layout, 0, 1,
                            &ctx->rcs_resources.sets[f].descset, 0, NULL);

    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);
    {
        float pushblock[1] = {
            1.0 // scale
        };
        vkCmdPushConstants(cmdbuf, ctx->rcs_resources.rcs_pipline_layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushblock),
                           pushblock);
    }
    vkCmdDrawIndexed(cmdbuf, ctx->rcs_resources.mesh.n_indices, 1, 0, 0, 0);

    if (ctx->rcs_resources.mesh.n_sharp_indices != 0) {
        vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          ctx->rcs_resources.ptd_pipeline);
        {
            float pushblock[1] = {
                1.01 // scale
            };
            vkCmdPushConstants(cmdbuf, ctx->rcs_resources.rcs_pipline_layout,
                               VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushblock),
                               pushblock);
        }

        vkCmdBindIndexBuffer(cmdbuf, ctx->rcs_resources.mesh.sharpindexbuf.buf,
                             0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmdbuf, ctx->rcs_resources.mesh.n_sharp_indices, 1, 0,
                         0, 0);
    }

    vkCmdEndRendering(cmdbuf);

    VkBuffer fft_buf = ctx->rcs_resources.sets[f].fft_work_buf.buf;

    /*
    This set of barriers ensures that rendertarget 0 is ready to be read from so
    that it can be copied into the two fft buffers
    */
    VkImageMemoryBarrier2 rend_to_comp_cp = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        ctx->rcs_resources.sets[f].rendtargets[0].img,
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VkDependencyInfo rend_to_comp_cp_dep = {
        VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
        NULL,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        &rend_to_comp_cp};

    u32 transfer_pushconsts = RCS_CROPFRACTION;

    vkCmdPipelineBarrier2(cmdbuf, &rend_to_comp_cp_dep);

    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                            ctx->rcs_resources.imgbuftransfer_pipeline_layout,
                            0, 1, &ctx->rcs_resources.sets[f].imgtobuf_descset,
                            0, NULL);
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                      ctx->rcs_resources.imgtobuf_pipeline);
    vkCmdPushConstants(
        cmdbuf, ctx->rcs_resources.imgbuftransfer_pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(u32), &transfer_pushconsts);
    vkCmdDispatch(cmdbuf, RCS_RESOLUTION / 16, RCS_RESOLUTION / 16, 1);

    /*
    This set of barriers ensures that the buffers are ready to be used by the
    VkFFT compute shader dispatches
    */
    VkBufferMemoryBarrier2 comp_cp_to_fftx = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        ctx->rcs_resources.sets[f].fft_work_buf.buf,
        0,
        VK_WHOLE_SIZE};

    VkBufferMemoryBarrier2 comp_cp_post_deps[1] = {comp_cp_to_fftx};

    VkDependencyInfo comp_cp_to_fft_dep = {
        VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
        NULL,
        0,
        0,
        NULL,
        1,
        comp_cp_post_deps,
        0,
        NULL};

    vkCmdPipelineBarrier2(cmdbuf, &comp_cp_to_fft_dep);

    /*
    After hours of experimentation, I think that doing an out-of place transform
    results in the same bugs as doing an in-place. So to save memory We'll just
    skip the manual temp buffer and use a single in place transform, just like
    in the beginning.
    */

    VkFFTLaunchParams lp = {};
    lp.commandBuffer = &cmdbuf;
    // lp.tempBuffer = &ctx->rcs_resources.sets[f].fft_buf_tmp.buf;
    lp.buffer = &ctx->rcs_resources.sets[f]
                     .fft_work_buf.buf; // this is the input for some reason
    // lp.inputBuffer = &ctx->rcs_resources.sets[f].fft_buf_input.buf;
    // lp.outputBuffer = &ctx->rcs_resources.sets[f].fft_buf_output.buf;

    VkFFTAppend(ctx->backend.fft[f], -1, &lp);

    /*
    This set of barriers ensures that
    1) the post-fft image is ready to be overwritten by the buffers
    2) the buffers are ready to be read from to write into the post-fft image
    */
    VkImageMemoryBarrier2 postfft_to_comp_img = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_NONE,
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        ctx->rcs_resources.sets[f].fft_img.img,
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VkBufferMemoryBarrier2 postfft_to_comp_cpx = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        ctx->rcs_resources.sets[f].fft_work_buf.buf,
        0,
        VK_WHOLE_SIZE,
    };

    VkBufferMemoryBarrier2 postfft_compcp_deps[1] = {postfft_to_comp_cpx};

    VkDependencyInfo postfft_to_compcp_dep = {
        VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
        NULL,
        0,
        0,
        NULL,
        1,
        postfft_compcp_deps,
        1,
        &postfft_to_comp_img};

    vkCmdPipelineBarrier2(cmdbuf, &postfft_to_compcp_dep);

    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                            ctx->rcs_resources.imgbuftransfer_pipeline_layout,
                            0, 1, &ctx->rcs_resources.sets[f].buftoimg_descset,
                            0, NULL);
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                      ctx->rcs_resources.buftoimg_pipeline);

    vkCmdPushConstants(
        cmdbuf, ctx->rcs_resources.imgbuftransfer_pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(u32), &transfer_pushconsts);
    vkCmdDispatch(cmdbuf, RCS_RESOLUTION / 16, RCS_RESOLUTION / 16, 1);

    /*
    This set of barriers ensures that the post-fft image can be read by the
    reduction compute shader
    */
    VkImageMemoryBarrier2 postcopy_readtohost_img = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        ctx->rcs_resources.sets[f].fft_img.img,
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VkDependencyInfo postcopy_readtohost_dep = {
        VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
        NULL,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        &postcopy_readtohost_img};

    vkCmdPipelineBarrier2(cmdbuf, &postcopy_readtohost_dep);

    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                      ctx->rcs_resources.reduction_pipeline);

    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                            ctx->rcs_resources.reduction_pipeline_layout, 0, 1,
                            &ctx->rcs_resources.sets[f].red_descset, 0, NULL);

    vkCmdPushConstants(
        cmdbuf, ctx->rcs_resources.reduction_pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(u32),
        &transfer_pushconsts); // should maybe get its own name, this isn't a
                               // transfer kernel but it needs the same data
    vkCmdDispatch(cmdbuf, 1, 1, 1);

    /*
    This set of barriers ensures two things:
    1) the reduction compute shader result can be read by the host
    2) All of the images needed for rendering to the screen are in the correct
    layouts
    */
    VkBufferMemoryBarrier2 extr = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                                   NULL,
                                   VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                   VK_ACCESS_2_SHADER_WRITE_BIT,
                                   VK_PIPELINE_STAGE_2_HOST_BIT,
                                   VK_ACCESS_2_HOST_READ_BIT,
                                   VK_QUEUE_FAMILY_IGNORED,
                                   VK_QUEUE_FAMILY_IGNORED,
                                   ctx->rcs_resources.sets[f].extr_buf.buf,
                                   0,
                                   VK_WHOLE_SIZE};

    VkImageMemoryBarrier2 before_rendertoscreen[4];

    for (u32 i = 0; i < 3; i++) {
        before_rendertoscreen[i] = (VkImageMemoryBarrier2){
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            NULL,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            ctx->rcs_resources.sets[f].rendtargets[i].img,
            (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    }

    before_rendertoscreen[0].srcStageMask =
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    before_rendertoscreen[0].srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    before_rendertoscreen[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;

    before_rendertoscreen[3] = (VkImageMemoryBarrier2){
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        ctx->rcs_resources.sets[f].fft_img.img,
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VkDependencyInfo post_extr_dep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                      NULL,
                                      0,
                                      0,
                                      NULL,
                                      1,
                                      &extr,
                                      4,
                                      before_rendertoscreen};

    vkCmdPipelineBarrier2(cmdbuf, &post_extr_dep);

    VkResult r = vkEndCommandBuffer(cmdbuf);
    assert(r == VK_SUCCESS);
}

void render_rcs_imgs(RenderContext* ctx, u32 f) {
    record_rcs_cmdbuf(ctx, f);

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 0;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &ctx->rcs_resources.sets[f].cmdbuf;
    si.signalSemaphoreCount = 0;

    VkResult r = vkQueueSubmit(ctx->backend.queues.graphics_queue, 1, &si,
                               VK_NULL_HANDLE);
    assert(r == VK_SUCCESS);
}