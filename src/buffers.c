#include "buffers.h"
#include "backend/backend.h"
#include "cleanupstack.h"
#include "common.h"
#include "vulkan/vulkan_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const Vertex verts[4] = {{{-0.5, -0.5}, {0.0, 0.0}},
                         {{-0.5, 0.5}, {0.0, 1.0}},
                         {{0.5, -0.5}, {1.0, 0.0}},
                         {{0.5, 0.5}, {1.0, 1.0}}};

const Vertex overview_verts[16] = {
    {{-1.0, -1.0}, {0.0, 0.0}}, {{+0.0, -1.0}, {1.0, 0.0}},
    {{-1.0, 0.0}, {0.0, 1.0}},  {{0.0, +0.0}, {1.0, 1.0}},

    {{0.0, -1.0}, {0.0, 0.0}},  {{+1.0, -1.0}, {1.0, 0.0}},
    {{+0.0, 0.0}, {0.0, 1.0}},  {{1.0, +0.0}, {1.0, 1.0}},

    {{-1.0, 0.0}, {0.0, 0.0}},  {{+0.0, 0.0}, {1.0, 0.0}},
    {{-1.0, 1.0}, {0.0, 1.0}},  {{0.0, 1.0}, {1.0, 1.0}},

    {{0.0, -0.0}, {0.0, 0.0}},  {{+1.0, 0.0}, {1.0, 0.0}},
    {{+0.0, 1.0}, {0.0, 1.0}},  {{1.0, +1.0}, {1.0, 1.0}},

};

const u16 overview_inds[] = {0,  1,  2,  1,
                             2,  3,

                             4,  5,  6,  5,
                             6,  7,

                             8,  9,  10, 9,
                             10, 11,

                             12, 13, 14, 13,
                             14, 15

};

const u16 indices[6] = {0, 1, 2, 1, 2, 3};

typedef struct BufferCleanInfo {
    VkDevice dev;
    VmaAllocation alloc;
    VmaAllocator allocctx;
    VkBuffer buf;
} BufferCleanInfo;

void destroy_buffer(void* obj) {
    BufferCleanInfo* b = (BufferCleanInfo*)obj;
    vmaDestroyBuffer(b->allocctx, b->buf, b->alloc);
}
/*
typedef struct DevmemCleanInfo {
    VkDevice dev;
    VkDeviceMemory mem;
} DevmemCleanInfo;

void destroy_devmem(void* obj) {
    DevmemCleanInfo* d = (DevmemCleanInfo*)obj;
    vkFreeMemory(d->dev, d->mem, NULL);
}

u32 find_memory_type(VkPhysicalDevice physdev, u32 typefilter,
VkMemoryPropertyFlags props) { VkPhysicalDeviceMemoryProperties memprops = {};
    vkGetPhysicalDeviceMemoryProperties(physdev, &memprops);

    for (u32 i = 0; i < memprops.memoryTypeCount; i++) {
        if (typefilter & (1 << i) && (memprops.memoryTypes[i].propertyFlags &
props) == props) { printf("selecting memory type index %u\n", i); return i;
        }
    }

    printf("NO SUITABLE MEMORY TYPE");
    abort();
}
*/

bool make_buffer(RenderBackend* rb, VkDeviceSize size, VkBufferUsageFlags usage,
                 Mappable mappable, Buffer* buf, CleanupStack* cs) {
    VkBufferCreateInfo bci = {};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = size;
    bci.usage = usage;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo aci = {};
    aci.usage = VMA_MEMORY_USAGE_AUTO;
    if (mappable == TR_MAPPABLE_WRITE) {
        aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else if (mappable == TR_MAPPABLE_READ) {
        aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VkResult r =
        vmaCreateBuffer(rb->alloc, &bci, &aci, &buf->buf, &buf->alloc, NULL);
    assert(r == VK_SUCCESS);

    CLEANUP_START_NORES(BufferCleanInfo){
        .dev = rb->dev,
        .alloc = buf->alloc,
        .allocctx = rb->alloc,
        .buf = buf->buf,
    };
    CLEANUP_END(buffer);

    return false;
}

void copybuffer(VkDevice dev, Queues queues, VkCommandPool pool, VkBuffer src,
                VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo cbi = {};
    cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbi.commandBufferCount = 1;
    cbi.commandPool = pool;
    cbi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer cbuf;
    vkAllocateCommandBuffers(dev, &cbi, &cbuf);

    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cbuf, &bi);

    VkBufferCopy copyreg = {};
    copyreg.size = size;
    copyreg.srcOffset = 0;
    copyreg.dstOffset = 0;

    vkCmdCopyBuffer(cbuf, src, dst, 1, &copyreg);

    vkEndCommandBuffer(cbuf);

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cbuf;

    vkQueueSubmit(queues.graphics_queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queues.graphics_queue);
    vkFreeCommandBuffers(dev, pool, 1, &cbuf);
}

bool make_local_buffer_staged(RenderBackend* rb, VkDeviceSize size,
                              const void* filldata, VkBufferUsageFlags usage,
                              VkCommandPool cpool, Buffer* buf,
                              CleanupStack* cs) {

    VkBuffer stagingbuf;
    VmaAllocation stagingalloc;

    VkBufferCreateInfo bci = {};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = size;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo aci = {};
    aci.usage = VMA_MEMORY_USAGE_AUTO;
    aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkResult r = vmaCreateBuffer(rb->alloc, &bci, &aci, &stagingbuf,
                                 &stagingalloc, NULL);
    assert(r == VK_SUCCESS);

    vmaSetAllocationName(rb->alloc, stagingalloc, "STAGING BUFFER");

    void* mapping;
    vmaMapMemory(rb->alloc, stagingalloc, &mapping);
    memcpy(mapping, filldata, size);
    vmaUnmapMemory(rb->alloc, stagingalloc);

    bci.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    aci.flags = 0;

    r = vmaCreateBuffer(rb->alloc, &bci, &aci, &buf->buf, &buf->alloc, NULL);
    assert(r == VK_SUCCESS);

    copybuffer(rb->dev, rb->queues, cpool, stagingbuf, buf->buf, size);

    vmaDestroyBuffer(rb->alloc, stagingbuf, stagingalloc);

    CLEANUP_START_NORES(BufferCleanInfo){
        .dev = rb->dev,
        .alloc = buf->alloc,
        .allocctx = rb->alloc,
        .buf = buf->buf,
    };
    CLEANUP_END(buffer);

    return false;
}

bool make_vertexbuffer(RenderBackend* rb, VkCommandPool pool, Buffer* vbuf,
                       CleanupStack* cs) {

    bool res = make_local_buffer_staged(
        rb, sizeof(overview_verts), overview_verts,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, pool, vbuf, cs);
    vmaSetAllocationName(rb->alloc, vbuf->alloc, "VERTEX BUFFER MAINSCREEN");
    return res;
}

bool make_indexbuffer(RenderBackend* rb, VkCommandPool pool, Buffer* ibuf,
                      CleanupStack* cs) {

    bool res = make_local_buffer_staged(
        rb, sizeof(overview_inds), overview_inds,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, pool, ibuf, cs);
    vmaSetAllocationName(rb->alloc, ibuf->alloc, "INDEX BUFFER MAINSCREEN");
    return res;
}
