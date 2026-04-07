#include "rcs/rcs_render.h"

#include "common.h"
#include "context.h"
#include "linalg.h"
#include "rcs/rcs_ubo.h"
#include "res.h"
#include <assert.h>

void write_rcs_ubo(RenderContext* ctx, void* mapping) {
    RcsUbo myubo = {};

    Mat4 ident4 = {{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0}};

    const f32 L = 1000.0f;

    myubo.model = ident4;
    myubo.view = ident4;
    myubo.proj = ident4;
    myubo.norm_trans = ident4;
    myubo.resolution_xy_L_ = (Vec4){RCS_RESOLUTION, RCS_RESOLUTION, L, 0.0};

    Mat4 scale = ident4;

    const f32 s = 0.5f;

    *pindex_m4(&scale, 0, 0) = s;
    *pindex_m4(&scale, 1, 1) = s;
    *pindex_m4(&scale, 2, 2) = s;

    Mat4 transl = ident4;

    *pindex_m4(&transl, 0, 3) = -0.0f;
    *pindex_m4(&transl, 1, 3) = 0.0f;
    *pindex_m4(&transl, 2, 3) = 0.0f;

    Mat4 rotx = ident4;
    Mat4 roty = ident4;
    static f32 angx = -10.0f * (3.141592f / 180.0f);
    static f32 angy = 0.0f * (3.141592f / 180.0f);

    angx += 0.1f * (3.1415 / 180);

    *pindex_m4(&rotx, 1, 1) = cosf(angx);
    *pindex_m4(&rotx, 1, 2) = -sinf(angx);
    *pindex_m4(&rotx, 2, 1) = sinf(angx);
    *pindex_m4(&rotx, 2, 2) = cosf(angx);

    *pindex_m4(&roty, 0, 0) = cosf(angy);
    *pindex_m4(&roty, 0, 2) = -sinf(angy);
    *pindex_m4(&roty, 2, 0) = sinf(angy);
    *pindex_m4(&roty, 2, 2) = cosf(angy);

    myubo.model = mul_m4(transl, mul_m4(rotx, mul_m4(roty, scale)));

    Mat3 norm = transpose_m3(invert_m3(subm4_m3(myubo.model)));

    Mat4 norm4 = zeroed_from_m3(norm);
    *pindex_m4(&norm4, 3, 3) = 1.0f; // set corner

    myubo.norm_trans = norm4;

    const f32 near = -100.0f, far = 100.0f, left = -10.0f, right = 10.0f,
              top = -10.0f, bot = 10.0f;

    *pindex_m4(&myubo.proj, 0, 0) = 2.0f / (right - left);
    *pindex_m4(&myubo.proj, 1, 1) = 2.0f / (bot - top);
    *pindex_m4(&myubo.proj, 2, 2) = 1.0f / (far - near);

    *pindex_m4(&myubo.proj, 0, 3) = -(right + left) / (right - left);
    *pindex_m4(&myubo.proj, 1, 3) = -(bot + top) / (bot - top);
    *pindex_m4(&myubo.proj, 2, 3) = -(near) / (far - near);

    memcpy(mapping, &myubo, sizeof(myubo));
}

// f is the inflight index
void record_rcs_cmdbuf(RenderContext* ctx, u32 f) {

    write_rcs_ubo(ctx, ctx->rcs_resources.sets[f].ubufmap);

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = 0;
    cbbi.pInheritanceInfo = NULL;

    VkClearValue clearcol = {};
    clearcol.color.float32[0] = 0.0f;
    clearcol.color.float32[1] = 0.0f;
    clearcol.color.float32[2] = 0.0f;
    clearcol.color.float32[3] = 1.0f;

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
        c[i].clearValue = clearcol;
        c[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        c[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        c[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        c[i].imageView = ctx->rcs_resources.sets[f].rendtargets[i].view;
    }

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
                      ctx->rcs_resources.pipeline);

    vkCmdBindVertexBuffers(cmdbuf, 0, 1, vbufs, vbuf_offsets);
    vkCmdBindIndexBuffer(cmdbuf, ctx->rcs_resources.mesh.indexbuf.buf, 0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            ctx->rcs_resources.pipeline_layout, 0, 1,
                            &ctx->rcs_resources.sets[f].descset, 0, NULL);

    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

    vkCmdDrawIndexed(cmdbuf, ctx->rcs_resources.mesh.n_indices, 1, 0, 0, 0);

    vkCmdEndRendering(cmdbuf);

    // barrier to transfer to fft buffer

    VkImageMemoryBarrier2 rendtocp = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COPY_BIT,
        VK_ACCESS_2_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        ctx->rcs_resources.sets[f].rendtargets[0].img,
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VkDependencyInfo rendtocpdep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
                                    NULL,
                                    0,
                                    0,
                                    NULL,
                                    0,
                                    NULL,
                                    1,
                                    &rendtocp};

    vkCmdPipelineBarrier2(cmdbuf, &rendtocpdep);

    VkBuffer fft_buf = ctx->rcs_resources.sets[f].fft_buf.buf;

    // the buffer copies can be configured to do the layout shifting to center
    // the fft

    VkBufferImageCopy quadrants[4] = {{}, {}, {}, {}};

    for (u32 i = 0; i < 4; i++) {
        quadrants[i].bufferImageHeight = (RCS_RESOLUTION);
        quadrants[i].bufferRowLength = (RCS_RESOLUTION);
        quadrants[i].imageExtent =
            (VkExtent3D){(RCS_RESOLUTION / 2), (RCS_RESOLUTION / 2), 1};
        quadrants[i].imageSubresource =
            (VkImageSubresourceLayers){VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    }
    const u32 texelsize = sizeof(float) * 2; // fft image is R32G32_SFLOAT
    // 0: top left to bottom right
    quadrants[0].imageOffset = (VkOffset3D){0, 0, 0};
    quadrants[0].bufferOffset =
        ((RCS_RESOLUTION) * (RCS_RESOLUTION / 2) + (RCS_RESOLUTION / 2)) *
        texelsize;

    // 1: top right to bottom left
    quadrants[1].imageOffset = (VkOffset3D){(RCS_RESOLUTION / 2), 0, 0};
    quadrants[1].bufferOffset =
        ((RCS_RESOLUTION) * (RCS_RESOLUTION / 2)) * texelsize;

    // 2: bottom left to top right
    quadrants[2].imageOffset = (VkOffset3D){0, (RCS_RESOLUTION / 2), 0};
    quadrants[2].bufferOffset = ((RCS_RESOLUTION / 2)) * texelsize;

    // 3: bottom right to top left
    quadrants[3].imageOffset =
        (VkOffset3D){RCS_RESOLUTION / 2, RCS_RESOLUTION / 2};
    quadrants[3].bufferOffset = 0;

    vkCmdCopyImageToBuffer(
        cmdbuf, ctx->rcs_resources.sets[f].rendtargets[0].img,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, fft_buf, 4, quadrants);

    VkBufferMemoryBarrier2 bufpostcp = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_COPY_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        fft_buf,
        0,
        VK_WHOLE_SIZE};

    VkDependencyInfo bufpostcpdep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
                                     NULL,
                                     0,
                                     0,
                                     NULL,
                                     1,
                                     &bufpostcp,
                                     0,
                                     NULL};

    vkCmdPipelineBarrier2(cmdbuf, &bufpostcpdep);

    VkFFTLaunchParams lp = {};
    lp.commandBuffer = &cmdbuf;
    lp.buffer = &fft_buf;

    VkFFTAppend(ctx->backend.fft[f], -1, &lp);

    VkBufferMemoryBarrier2 bufpostfft = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COPY_BIT,
        VK_ACCESS_2_TRANSFER_READ_BIT,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        fft_buf,
        0,
        VK_WHOLE_SIZE};

    VkImageMemoryBarrier2 imgpostfft = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_NONE,
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_COPY_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        ctx->rcs_resources.sets[f].fft_img.img,
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VkDependencyInfo postfftdep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                   NULL,
                                   0,
                                   0,
                                   NULL,
                                   1,
                                   &bufpostfft,
                                   1,
                                   &imgpostfft};

    vkCmdPipelineBarrier2(cmdbuf, &postfftdep);

    vkCmdCopyBufferToImage(cmdbuf, fft_buf,
                           ctx->rcs_resources.sets[f].fft_img.img,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 4, quadrants);

    VkImageMemoryBarrier2 postfftcp[4];

    for (u32 i = 0; i < 3; i++) {
        postfftcp[i] = (VkImageMemoryBarrier2){
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

    postfftcp[0].srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    postfftcp[0].srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    postfftcp[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    postfftcp[3] = (VkImageMemoryBarrier2){
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        NULL,
        VK_PIPELINE_STAGE_2_COPY_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        ctx->rcs_resources.sets[f].fft_img.img,
        (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VkDependencyInfo postfftcpdep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                     NULL,
                                     0,
                                     0,
                                     NULL,
                                     0,
                                     NULL,
                                     4,
                                     postfftcp};

    vkCmdPipelineBarrier2(cmdbuf, &postfftcpdep);

    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                      ctx->rcs_resources.reduction_pipeline);

    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                            ctx->rcs_resources.reduction_pipeline_layout, 0, 1,
                            &ctx->rcs_resources.sets[f].red_descset, 0, NULL);

    vkCmdDispatch(cmdbuf, 1, 1, 1);

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

    VkDependencyInfo extrdep = {
        VK_STRUCTURE_TYPE_DEPENDENCY_INFO, NULL, 0, 0, NULL, 1, &extr, 0, NULL};

    vkCmdPipelineBarrier2(cmdbuf, &extrdep);

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