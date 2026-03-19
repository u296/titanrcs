#include "cleanupstack.h"
#include "common.h"
#include "backend/backend.h"
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

bool make_commandpool(VkDevice dev, Queues queues, VkCommandPool* pool, struct Error* e_out, CleanupStack* cs) {
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

