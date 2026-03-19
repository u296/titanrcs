#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "cleanupstack.h"
#include "common.h"
#include "linalg.h"


typedef struct UniformBufferObject {
    Mat4 model;
    Mat4 view;
    Mat4 proj;
} UniformBufferObject;

bool make_descriptorsetlayout(VkDevice dev, VkDescriptorSetLayout* desc_layout, CleanupStack* cs);
bool make_uniform_buffers(const u32 n_max_inflight, VkPhysicalDevice physdev, VkDevice dev, Buffer** ubufs, void*** ubuf_mappings, Error* e_out, CleanupStack* cs);

void update_uniformbuffer(u64 frame, VkExtent2D swp_ext, void* ubufmap);

bool make_descriptor_pool(const u32 n_max_inflight, VkDevice dev, VkDescriptorPool* dpool, Error* e_out, CleanupStack* cs);

bool make_descriptor_sets(const u32 n_max_inflight, VkDevice dev, VkDescriptorPool dpool, Buffer* ubufs, VkDescriptorSetLayout desc_layout, VkDescriptorSet** desc_sets, Error*e_out, CleanupStack*cs);
#endif