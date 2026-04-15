#ifndef COMMON_H
#define COMMON_H
#include <stdbool.h>
#include <stdint.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;

typedef float f32;
typedef double f64;

typedef uintptr_t usize;

constexpr f32 PI = 3.1415926535f;
constexpr f32 DEG_TO_RAD = 0.01745329251f;

#define CLAMP(a, min, max) (a < min ? min : (a > max ? max : a))

#define VERIFY(o, r)                                                                               \
    if (r != VK_SUCCESS) {                                                                         \
        *e_out = (struct Error){.origin = o, .code = r};                                           \
        return true;                                                                               \
    }

typedef struct Error {
    const char* origin;
    VkResult code;
} Error;

/*
typedef struct Buffer {
    VkBuffer buf;
    VkDeviceMemory mem;
    VkDeviceSize offset;
} Buffer;
*/

typedef struct Image {
    VkImage img;
    VkImageView view;
    VmaAllocation alloc;
} Image;

typedef struct Buffer {
    VkBuffer buf;
    VkBufferView view;
    VmaAllocation alloc;
} Buffer;

typedef struct RcsRenderMesh {
    Buffer vertexbuf;
    Buffer indexbuf;
    Buffer sharpindexbuf;
    u32 n_indices;
    u32 n_sharp_indices;
} RcsRenderMesh;

#endif