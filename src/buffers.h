#ifndef VERTEXBUF_H
#define VERTEXBUF_H
#include "cleanupstack.h"
#include "common.h"
#include "device.h"

typedef struct Vec2 {
    float x;
    float y;
} Vec2;

typedef struct Vec3 {
    float x;
    float y;
    float z;
} Vec3;


constexpr u32 N_VERT_ATTRIB = 2;

typedef struct Vertex {
    Vec2 pos;
    Vec3 col;
}  Vertex;

constexpr VkVertexInputBindingDescription vertex_binding_desc = {
    0,
    sizeof(Vertex),
    VK_VERTEX_INPUT_RATE_VERTEX
};

constexpr VkVertexInputAttributeDescription vertex_attrib_desc[N_VERT_ATTRIB] = {
    {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos)},
    {1,0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, col)}
};

bool make_buffer(VkPhysicalDevice physdev, VkDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memprops, Buffer* buf, Error* e_out, CleanupStack* cs);

bool make_vertexbuffer(VkPhysicalDevice physdev, VkDevice dev, Queues queues, VkCommandPool pool, Buffer* vbuf, Error* e_out, CleanupStack* cs);
bool make_indexbuffer(VkPhysicalDevice physdev, VkDevice dev, Queues queues, VkCommandPool pool, Buffer* ibuf, Error* e_out, CleanupStack* cs);

#endif