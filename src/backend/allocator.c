#include "backend/backend.h"
#include <assert.h>
#include <volk.h>

#include <vk_mem_alloc.h>

bool make_allocator(RenderBackend*rb) {
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


    return false;
}