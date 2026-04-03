#include "rcs/rcs_cmdbuf.h"
#include "backend/backend.h"
#include "cleanupstack.h"
#include "common.h"

bool make_rcs_cmdbuf(RenderBackend* rb, VkCommandPool cpool,
                     VkCommandBuffer* out_cmdbuf, CleanupStack* cs) {

    VkCommandBufferAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = cpool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;

    vkAllocateCommandBuffers(rb->dev, &ai, out_cmdbuf);

    return false;
}