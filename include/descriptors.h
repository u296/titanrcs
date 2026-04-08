#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "cleanupstack.h"
#include "common.h"
#include "linalg.h"
#include "rcs/rcs.h"

typedef struct UniformBufferObject {
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    Vec4 fzoom_;
} InterfaceUbo;

bool make_descriptorsetlayout(VkDevice dev, VkDescriptorSetLayout* desc_layout, CleanupStack* cs);

void write_interface_ubo(u64 frame, VkExtent2D swp_ext, void* ubufmap, f32 ctxzoom);

bool make_descriptor_pool(const u32 n_max_inflight, VkDevice dev, VkDescriptorPool* dpool,
                          Error* e_out, CleanupStack* cs);


bool make_samplers(RenderBackend* rb, VkSampler* out_sampler, CleanupStack* cs);

bool make_descriptor_sets(const u32 n_max_inflight, VkDevice dev,
                          VkDescriptorPool dpool, Buffer* ubufs,
                          VkDescriptorSetLayout desc_layout,
                          RcsResources* rcs_res, VkDescriptorSet** desc_sets,
                          Error* e_out, CleanupStack* cs);
#endif