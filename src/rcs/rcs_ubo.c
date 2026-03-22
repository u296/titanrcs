
#include "rcs/rcs_ubo.h"
#include "backend/backend.h"
#include "cleanupstack.h"


bool make_rcs_ubo(RenderBackend* rb, CleanupStack* cs) {

    VkBufferCreateInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = sizeof(RcsUbo);
    bi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo aci = {};
    aci.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBuffer(rb->alloc, &bi, &aci, VkBuffer  _Nullable * _Nonnull pBuffer, VmaAllocation  _Nullable * _Nonnull pAllocation, VmaAllocationInfo * _Nullable pAllocationInfo)

    return false;
}