#include "backend/backend.h"
#include "cleanupstack.h"
#include <assert.h>
#include <volk.h>

#include <vk_mem_alloc.h>

void destroy_allocator(void* obj) {
    VmaAllocator alloc = *(VmaAllocator*)obj;

    char* stats;

    vmaBuildStatsString(alloc, &stats, VK_TRUE);

    //printf("VMA STATS right before destroy:\n%s\n", stats);

    vmaDestroyAllocator(alloc);
}

bool make_allocator(RenderBackend*rb, CleanupStack* cs) {
    VmaAllocatorCreateInfo aci = {};
    aci.instance = rb->inst;
    aci.device = rb->dev;
    aci.physicalDevice = rb->physdev;
    aci.vulkanApiVersion = VK_API_VERSION_1_0;

    VmaVulkanFunctions vkfn;
    VkResult res = vmaImportVulkanFunctionsFromVolk(&aci, &vkfn);
    assert(res == VK_SUCCESS);

    aci.pVulkanFunctions = &vkfn;

    res = vmaCreateAllocator(&aci, &rb->alloc);

    CLEANUP_START_NORES(VmaAllocator)
    rb->alloc
    CLEANUP_END(allocator)


    return false;
}