#include "buffers.h"
#include "cleanupstack.h"
#include "common.h"
#include "device.h"
#include "vulkan/vulkan_core.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



const Vertex verts[4] = {
    {{-0.5,-0.5},{1.0,1.0,0.0}},
    {{-0.5,0.5},{0.0,1.0,0.0}},
    {{0.5,-0.5},{0.0,0.0,1.0}},
    {{0.5,0.5}, {1.0,1.0,1.0}}
};

const u16 indices[6] = {
    0,1,2,
    1,2,3
};

typedef struct BufferCleanInfo {
    VkDevice dev;
    VkBuffer buf;
} BufferCleanInfo;

void destroy_buffer(void* obj) {
    BufferCleanInfo* b = (BufferCleanInfo*)obj;
    vkDestroyBuffer(b->dev, b->buf, NULL);
}

typedef struct DevmemCleanInfo {
    VkDevice dev;
    VkDeviceMemory mem;
} DevmemCleanInfo;

void destroy_devmem(void* obj) {
    DevmemCleanInfo* d = (DevmemCleanInfo*)obj;
    vkFreeMemory(d->dev, d->mem, NULL);
}


u32 find_memory_type(VkPhysicalDevice physdev, u32 typefilter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memprops = {};
    vkGetPhysicalDeviceMemoryProperties(physdev, &memprops);

    for (u32 i = 0; i < memprops.memoryTypeCount; i++) {
        if (typefilter & (1 << i) && (memprops.memoryTypes[i].propertyFlags & props) == props) {
            printf("selecting memory type index %u\n", i);
            return i;
        }
    }

    printf("NO SUITABLE MEMORY TYPE");
    abort();
}

bool make_buffer(VkPhysicalDevice physdev, VkDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memprops, Buffer* buf, Error* e_out, CleanupStack* cs) {
    VkBufferCreateInfo bci = {};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = size;
    bci.usage = usage;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult r = vkCreateBuffer(dev, &bci, NULL, &buf->buf);
    CLEANUP_START(BufferCleanInfo)
    {dev,buf->buf}
    CLEANUP_END(buffer)

    VERIFY("buffer",r)

    VkMemoryRequirements mem_req = {};
    vkGetBufferMemoryRequirements(dev, buf->buf,&mem_req);

    u32 memtype_i = find_memory_type(physdev, mem_req.memoryTypeBits, memprops);

    VkMemoryAllocateInfo mai = {};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.memoryTypeIndex = memtype_i;
    mai.allocationSize = mem_req.size;
    mai.pNext = NULL;

    r = vkAllocateMemory(dev, &mai, NULL, &buf->mem);
    CLEANUP_START(DevmemCleanInfo)
    {dev, buf->mem}
    CLEANUP_END(devmem)

    VERIFY("device memory allocation", r)

    vkBindBufferMemory(dev, buf->buf, buf->mem, 0);

    return false;
}

void copybuffer(VkDevice dev, Queues queues, VkCommandPool pool, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
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
    
    vkCmdCopyBuffer(cbuf,src, dst, 1, &copyreg);

    vkEndCommandBuffer(cbuf);

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cbuf;

    vkQueueSubmit(queues.graphics_queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queues.graphics_queue);
    vkFreeCommandBuffers(dev, pool, 1, &cbuf);
}

bool make_local_buffer_staged(VkDeviceSize size, const void* filldata, VkBufferUsageFlags usage, VkPhysicalDevice physdev, VkDevice dev, Queues queues, VkCommandPool pool, Buffer* buf, Error* e_out, CleanupStack* cs) {
    Buffer stagingbuf = {};
    VkResult r = make_buffer(physdev, dev, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &stagingbuf, e_out, NULL)? VK_ERROR_UNKNOWN : VK_SUCCESS;
    VERIFY("staging buffer", r)

    void* data;
    vkMapMemory(dev, stagingbuf.mem, 0, size, 0, &data);
    memcpy(data, filldata, size);
    vkUnmapMemory(dev, stagingbuf.mem);

    r = make_buffer(physdev, dev, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buf, e_out, cs)? VK_ERROR_UNKNOWN : VK_SUCCESS;
    VERIFY("vertex buffer make", r);

    copybuffer(dev,queues,pool,stagingbuf.buf,buf->buf,size);

    vkDestroyBuffer(dev, stagingbuf.buf, NULL);
    vkFreeMemory(dev,stagingbuf.mem,NULL);

    return false;
}


bool make_vertexbuffer(VkPhysicalDevice physdev, VkDevice dev, Queues queues, VkCommandPool pool, Buffer* vbuf, Error* e_out, CleanupStack* cs) {


    return make_local_buffer_staged(sizeof(verts),verts, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, physdev, dev, queues, pool, vbuf, e_out, cs);
}

bool make_indexbuffer(VkPhysicalDevice physdev, VkDevice dev, Queues queues, VkCommandPool pool, Buffer* ibuf, Error* e_out, CleanupStack* cs) {


    return make_local_buffer_staged(sizeof(indices),indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, physdev, dev, queues, pool, ibuf, e_out, cs);
}
