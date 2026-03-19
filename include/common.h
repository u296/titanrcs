#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#include <volk.h>
#include <stdbool.h>

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

#define CLAMP(a,min,max) (a < min ? min : (a > max ? max : a))

#define VERIFY(o,r) \
if (r != VK_SUCCESS) { \
    *e_out = (struct Error){.origin=o,.code=r}; \
    return true;\
}

typedef struct Error {
    const char* origin;
    VkResult code;
} Error;

typedef struct Buffer {
	VkBuffer buf;
	VkDeviceMemory mem;
	VkDeviceSize offset;
} Buffer;

typedef struct Renderable {
	Buffer vertexbuf;
    Buffer indexbuf;
	VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
} Renderable;

#endif