#include "descriptors.h"
#include "cleanupstack.h"
#include "common.h"
#include "vulkan/vulkan_core.h"
#include <stdlib.h>
#include <string.h>

typedef struct DescriptorSetLayoutCleanup {
    VkDevice dev;
    VkDescriptorSetLayout desc_layout;
} DescriptorSetLayoutCleanup;

void destroy_descriptor_set_layout(void* obj) {
    DescriptorSetLayoutCleanup* c = (DescriptorSetLayoutCleanup*)obj;
    vkDestroyDescriptorSetLayout(c->dev, c->desc_layout, NULL);
}

bool make_descriptorsetlayout(VkDevice dev, VkDescriptorSetLayout* desc_layout, CleanupStack* cs) {
    VkDescriptorSetLayoutBinding dslb = {};
    dslb.binding = 0;
    dslb.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dslb.descriptorCount = 1;
    dslb.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    dslb.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo dsli = {};
    dsli.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsli.bindingCount = 1;
    dsli.pBindings = &dslb;

    VkResult r = vkCreateDescriptorSetLayout(dev, &dsli, NULL, desc_layout);
    CLEANUP_START(DescriptorSetLayoutCleanup){dev, *desc_layout} CLEANUP_END(descriptor_set_layout)

        return false;
}

typedef struct DescriptorPoolCleanup {
    VkDevice dev;
    VkDescriptorPool dpool;
} DescriptorPoolCleanup;

void destroy_dpool(void* obj) {
    DescriptorPoolCleanup* d = (DescriptorPoolCleanup*)obj;
    vkDestroyDescriptorPool(d->dev, d->dpool, NULL);
}

bool make_descriptor_pool(const u32 n_max_inflight, VkDevice dev, VkDescriptorPool* dpool,
                          Error* e_out, CleanupStack* cs) {
    VkDescriptorPoolSize ps = {};
    ps.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ps.descriptorCount = n_max_inflight;

    VkDescriptorPoolCreateInfo pci = {};
    pci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pci.poolSizeCount = 1;
    pci.pPoolSizes = &ps;
    pci.maxSets = n_max_inflight;

    VkResult r = vkCreateDescriptorPool(dev, &pci, NULL, dpool);
    CLEANUP_START(DescriptorPoolCleanup){dev, *dpool} CLEANUP_END(dpool)

        VERIFY("dpool", r)

            return false;
}

bool make_descriptor_sets(const u32 n_max_inflight, VkDevice dev, VkDescriptorPool dpool,
                          Buffer* ubufs, VkDescriptorSetLayout desc_layout,
                          VkDescriptorSet** desc_sets, Error* e_out, CleanupStack* cs) {

    *desc_sets = malloc(sizeof(VkDescriptorSet) * n_max_inflight);

    CLEANUP_START_NORES(void*)
    *desc_sets CLEANUP_END(memfree)

        VkDescriptorSetLayout* desc_layout_copies =
        malloc(sizeof(VkDescriptorSetLayout) * n_max_inflight);
    for (u32 i = 0; i < n_max_inflight; i++) {
        memcpy(desc_layout_copies + i, &desc_layout, sizeof(VkDescriptorSetLayout));
    }

    VkDescriptorSetAllocateInfo dai = {};
    dai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dai.descriptorPool = dpool;
    dai.descriptorSetCount = n_max_inflight;
    dai.pSetLayouts = desc_layout_copies;

    VkResult r = vkAllocateDescriptorSets(dev, &dai, *desc_sets);

    VERIFY("allocate descriptor sets", r)

    for (u32 i = 0; i < n_max_inflight; i++) {
        VkDescriptorBufferInfo dbi = {};
        dbi.buffer = ubufs[i].buf;
        dbi.offset = 0;
        dbi.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet wds = {};
        wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds.dstSet = (*desc_sets)[i];
        wds.dstBinding = 0;
        wds.dstArrayElement = 0;
        wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        wds.descriptorCount = 1;
        wds.pBufferInfo = &dbi;
        wds.pImageInfo = NULL;
        wds.pTexelBufferView = NULL;

        vkUpdateDescriptorSets(dev, 1, &wds, 0, NULL);
    }

    free(desc_layout_copies);

    return false;
}
