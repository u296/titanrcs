#ifndef RCS_PIPELINE_H
#define RCS_PIPELINE_H

#include "common.h"

typedef struct Vec2 {
    float x;
    float y;
} Vec2;

typedef struct Vec3 {
    float x;
    float y;
    float z;
} Vec3;

constexpr u32 N_RCSVERT_ATTRIB = 2;

typedef struct RcsVertex {
    Vec3 pos;
    Vec3 normal;
} Vertex;

constexpr VkVertexInputAttributeDescription RCSVERT_ATTRIB_DESC[N_RCSVERT_ATTRIB] = {
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)}};

#endif