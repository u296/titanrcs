#include<volk.h>
#include <vk_mem_alloc.h>

void make_allocator(VkInstance inst, VkPhysicalDevice physdev, VkDevice dev) {
    VmaAllocatorCreateInfo aci = {};
    aci.instance = inst;
    aci.device = dev;
    aci.physicalDevice = physdev;
    aci.vulkanApiVersion = VK_API_VERSION_1_0;

    VmaVulkanFunctions vkfn;
    VkResult res = vmaImportVulkanFunctionsFromVolk(&aci, &vkfn);

    aci.pVulkanFunctions = &vkfn;

    VmaAllocator alloc;
    res = vmaCreateAllocator(&aci, &alloc);
}