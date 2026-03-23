#ifndef VERTEXBUF_H
#define VERTEXBUF_H
#include "backend/backend.h"
#include "cleanupstack.h"
#include "common.h"
#include "linalg.h"

constexpr u32 N_VERT_ATTRIB = 2;

typedef struct Vertex {
    Vec2 pos;
    Vec3 col;
} Vertex;

constexpr VkVertexInputBindingDescription vertex_binding_desc = {0, sizeof(Vertex),
                                                                 VK_VERTEX_INPUT_RATE_VERTEX};

constexpr VkVertexInputAttributeDescription vertex_attrib_desc[N_VERT_ATTRIB] = {
    {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos)},
    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, col)}};

bool make_buffer(RenderBackend* rb, VkDeviceSize size, VkBufferUsageFlags usage, bool mappable,
                 Buffer* buf, CleanupStack* cs);

bool make_local_buffer_staged(RenderBackend* rb, VkDeviceSize size, const void* filldata,
                              VkBufferUsageFlags usage, VkCommandPool cpool, Buffer* buf,
                              CleanupStack* cs);

bool make_vertexbuffer(RenderBackend* rb, VkCommandPool pool, Buffer* ibuf, CleanupStack* cs);
bool make_indexbuffer(RenderBackend* rb, VkCommandPool pool, Buffer* ibuf, CleanupStack* cs);

#endif