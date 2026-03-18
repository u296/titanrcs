#include "commandpool.h"
#include "cleanupstack.h"
#include "common.h"
#include "device.h"
#include "vulkan/vulkan_core.h"
#include<stdlib.h>

typedef struct CommandpoolCleanup {
    VkDevice dev;
    VkCommandPool pool;
} CommandpoolCleanup;

void destroy_commandpool(void* obj) {
    struct CommandpoolCleanup* cc = (struct CommandpoolCleanup*)obj;
    vkDestroyCommandPool(cc->dev, cc->pool, NULL);
}

bool make_commandpool(VkDevice dev, struct Queues queues, VkCommandPool* pool, struct Error* e_out, CleanupStack* cs) {
    VkCommandPoolCreateInfo cpi = {};
    cpi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpi.queueFamilyIndex = queues.i_graphics_queue_fam;

    VkResult r = vkCreateCommandPool(dev, &cpi, NULL, pool);

    CLEANUP_START(CommandpoolCleanup)
    {dev,*pool}
    CLEANUP_END(commandpool)

    VERIFY("commandpool", r)
    return false;
}

bool make_commandbuffers(VkDevice dev, VkCommandPool pool, u32 n_max_inflight, VkCommandBuffer** cmdbufs, struct Error* e_out, CleanupStack* cs) {
    
    *cmdbufs = malloc(sizeof(VkCommandBuffer)*n_max_inflight);
    CLEANUP_START_NORES(void*)
    *cmdbufs
    CLEANUP_END(memfree)
    
    VkCommandBufferAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = n_max_inflight;

    VkResult r = vkAllocateCommandBuffers(dev, &ai, *cmdbufs);
    VERIFY("cmdbuf", r)

    return false;

}

bool recordcommandbuffer(VkExtent2D swapchainextent, VkFramebuffer fb, VkCommandBuffer cmdbuf, VkRenderPass renderpass, VkDescriptorSet desc_set, Renderable ren, struct Error* e_out) {

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = 0;
    cbbi.pInheritanceInfo = NULL;


    VkRenderPassBeginInfo rpbi = {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = renderpass;
    rpbi.framebuffer = fb;
    rpbi.renderArea.offset = (struct VkOffset2D){0,0};
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
    scissor.offset = (struct VkOffset2D){0,0};

    VkBuffer vbufs[] = {ren.vertexbuf.buf};
    VkDeviceSize vbuf_offsets[] = {0};

    vkBeginCommandBuffer(cmdbuf, &cbbi);


    vkCmdBeginRenderPass(cmdbuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ren.pipeline);

    vkCmdBindVertexBuffers(cmdbuf,0,1, vbufs, vbuf_offsets);
    vkCmdBindIndexBuffer(cmdbuf, ren.indexbuf.buf, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ren.pipeline_layout, 0, 1, &desc_set, 0, NULL);

    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

    vkCmdDrawIndexed(cmdbuf,6,1,0,0,0);
    //vkCmdDraw(cmdbuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdbuf);

    VkResult r = vkEndCommandBuffer(cmdbuf);
    VERIFY("recording", r)

    return false;
}