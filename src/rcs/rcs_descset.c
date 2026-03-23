#include "rcs/rcs_descset.h"

#include "backend/backend.h"
#include "cleanupdb.h"
#include "context.h"
#include "rcs/rcs_ubo.h"
#include <assert.h>

bool make_rcs_dpool(VkDevice dev, VkDescriptorPool* dpool, CleanupStack* cs) {

    VkDescriptorPoolSize ubo_ps = {};
    ubo_ps.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_ps.descriptorCount = 1;

    VkDescriptorPoolSize sampler_ps = {};
    sampler_ps.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_ps.descriptorCount = 1;

    VkDescriptorPoolSize pool_sizes[] = {ubo_ps, sampler_ps};

    VkDescriptorPoolCreateInfo dpci = {};
    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.poolSizeCount = 2;
    dpci.pPoolSizes = pool_sizes;
    dpci.maxSets = 1;

    VkResult r = vkCreateDescriptorPool(dev, &dpci, NULL, dpool);
    CLEANUP_START(DescriptorPoolCleanup){dev, *dpool} CLEANUP_END(dpool)

        return false;
}

bool make_rcs_descset_layout(VkDevice dev, VkDescriptorSetLayout* desc_layout, CleanupStack* cs) {
    VkDescriptorSetLayoutBinding vert_dslb = {};
    vert_dslb.binding = 0;
    vert_dslb.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vert_dslb.descriptorCount = 1;
    vert_dslb.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    vert_dslb.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding frag_dslb = {};
    frag_dslb.binding = 1;
    frag_dslb.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    frag_dslb.descriptorCount = 1;
    frag_dslb.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_dslb.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo dsli = {};
    dsli.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsli.bindingCount = 2;
    dsli.pBindings = (VkDescriptorSetLayoutBinding[]){vert_dslb, frag_dslb};

    VkResult r = vkCreateDescriptorSetLayout(dev, &dsli, NULL, desc_layout);
    CLEANUP_START(DescriptorSetLayoutCleanup){dev, *desc_layout} CLEANUP_END(descriptor_set_layout)

        return false;
}

bool make_rcs_descset(RenderBackend* rb, VkDescriptorPool dpool,
                      VkDescriptorSetLayout descset_layout, Buffer ubo, VkDescriptorSet* desc_set) {

    VkDescriptorSetAllocateInfo dsai = {};
    dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.descriptorPool = dpool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &descset_layout;

    VkResult r = vkAllocateDescriptorSets(rb->dev, &dsai, desc_set);
    assert(r == VK_SUCCESS);

    VkDescriptorBufferInfo dbi = {};
    dbi.buffer = ubo.buf;
    dbi.offset = 0;
    dbi.range = sizeof(RcsUbo);

    VkWriteDescriptorSet wds = {};
    wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    wds.dstSet = *desc_set;
    wds.dstBinding = 0;
    wds.dstArrayElement = 0;
    wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    wds.descriptorCount = 1;
    wds.pBufferInfo = &dbi;
    wds.pImageInfo = NULL;
    wds.pTexelBufferView = NULL;

    vkUpdateDescriptorSets(rb->dev, 1, &wds, 0, NULL);

    return false;
}