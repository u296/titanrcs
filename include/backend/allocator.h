#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "vk_mem_alloc.h"

void make_allocator(VkInstance inst, VkPhysicalDevice physdev, VkDevice dev,
                    VmaAllocator* out_alloc);

#endif
