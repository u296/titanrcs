#include "rcs/rcs_descset.h"

#include "backend/backend.h"
#include "cleanupdb.h"
#include "context.h"
#include "rcs/rcs_ubo.h"
#include "res.h"
#include <assert.h>

bool make_rcs_dpool(VkDevice dev, VkDescriptorPool* dpool, CleanupStack* cs) {

    VkDescriptorPoolSize ubo_ps = {};
    ubo_ps.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_ps.descriptorCount = N_MAX_INFLIGHT;

    VkDescriptorPoolSize sampler_ps = {};
    sampler_ps.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_ps.descriptorCount = N_MAX_INFLIGHT * (1 + 1);

    VkDescriptorPoolSize ssbo_ps = {};
    ssbo_ps.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssbo_ps.descriptorCount = N_MAX_INFLIGHT * (1 + 2*2);

    VkDescriptorPoolSize str_img_ps = {};
    str_img_ps.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    str_img_ps.descriptorCount = N_MAX_INFLIGHT * (2);

    VkDescriptorPoolSize pool_sizes[] = {ubo_ps, sampler_ps, ssbo_ps, str_img_ps};

    VkDescriptorPoolCreateInfo dpci = {};
    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.poolSizeCount = 4;
    dpci.pPoolSizes = pool_sizes;
    dpci.maxSets = N_MAX_INFLIGHT * (1 + 1 + 2);

    VkResult r = vkCreateDescriptorPool(dev, &dpci, NULL, dpool);
    CLEANUP_START(DescriptorPoolCleanup){dev, *dpool} CLEANUP_END(dpool)

        return false;
}

bool make_rcs_po_descset_layout(VkDevice dev, VkDescriptorSetLayout* desc_layout,
                             CleanupStack* cs) {
    VkDescriptorSetLayoutBinding vert_dslb = {};
    vert_dslb.binding = 0;
    vert_dslb.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vert_dslb.descriptorCount = 1;
    vert_dslb.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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
    CLEANUP_START(DescriptorSetLayoutCleanup){dev, *desc_layout} CLEANUP_END(
        descriptor_set_layout)

        return false;
}

bool make_rcs_descset(RenderBackend* rb, VkDescriptorPool dpool,
                      VkDescriptorSetLayout descset_layout, Buffer ubo,
                      VkDescriptorSet* desc_set) {

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

bool make_rcs_red_descset_layout(VkDevice dev,
                                 VkDescriptorSetLayout* desc_layout,
                                 CleanupStack* cs) {
    VkDescriptorSetLayoutBinding infftimg = {};
    infftimg.binding = 0;
    infftimg.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    infftimg.descriptorCount = 1;
    infftimg.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    infftimg.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding outssbo = {};
    outssbo.binding = 1;
    outssbo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outssbo.descriptorCount = 1;
    outssbo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    outssbo.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo dsli = {};
    dsli.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsli.bindingCount = 2;
    dsli.pBindings = (VkDescriptorSetLayoutBinding[]){infftimg, outssbo};

    VkResult r = vkCreateDescriptorSetLayout(dev, &dsli, NULL, desc_layout);
    CLEANUP_START(DescriptorSetLayoutCleanup){dev, *desc_layout} CLEANUP_END(
        descriptor_set_layout)

        return false;
}

bool make_rcs_red_descset(RenderBackend* rb, VkDescriptorPool dpool,
                          Image fftimg, VkSampler sampler, Buffer extr_ssbo,
                          VkDescriptorSetLayout descset_layout,
                          VkDescriptorSet* desc_set) {

    VkDescriptorSetAllocateInfo dsai = {};
    dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.descriptorPool = dpool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &descset_layout;

    VkResult r = vkAllocateDescriptorSets(rb->dev, &dsai, desc_set);
    assert(r == VK_SUCCESS);

    VkDescriptorImageInfo imi = {};
    imi.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imi.imageView = fftimg.view;
    imi.sampler = sampler;

    VkDescriptorBufferInfo dbi = {};
    dbi.buffer = extr_ssbo.buf;
    dbi.offset = 0;
    dbi.range = sizeof(ExtractionSsbo);

    VkWriteDescriptorSet img_wds = {};
    img_wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    img_wds.dstSet = *desc_set;
    img_wds.dstBinding = 0;
    img_wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    img_wds.descriptorCount = 1;
    img_wds.pImageInfo = &imi;

    VkWriteDescriptorSet buf_wds = {};
    buf_wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    buf_wds.dstSet = *desc_set;
    buf_wds.dstBinding = 1;
    buf_wds.dstArrayElement = 0;
    buf_wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    buf_wds.descriptorCount = 1;
    buf_wds.pBufferInfo = &dbi;
    buf_wds.pImageInfo = NULL;
    buf_wds.pTexelBufferView = NULL;

    VkWriteDescriptorSet writes[2] = {img_wds, buf_wds};

    vkUpdateDescriptorSets(rb->dev, 2, writes, 0, NULL);

    return false;
}


bool make_rcs_imgtobuf_descset_layout(VkDevice dev,
                                 VkDescriptorSetLayout* desc_layout,
                                 CleanupStack* cs) {
    VkDescriptorSetLayoutBinding image = {};
    image.binding = 0;
    image.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    image.descriptorCount = 1;
    image.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    image.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding outssbox = {};
    outssbox.binding = 1;
    outssbox.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outssbox.descriptorCount = 1;
    outssbox.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    outssbox.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo dsli = {};
    dsli.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsli.bindingCount = 2;
    dsli.pBindings = (VkDescriptorSetLayoutBinding[]){image, outssbox};

    VkResult r = vkCreateDescriptorSetLayout(dev, &dsli, NULL, desc_layout);
    CLEANUP_START(DescriptorSetLayoutCleanup){dev, *desc_layout} CLEANUP_END(
        descriptor_set_layout)

        return false;
}

bool make_rcs_imgtobuf_descset(RenderBackend* rb, VkDescriptorPool dpool,
                          Image prefftimg, Buffer fftbuf_x, Buffer fftbuf_y,
                          VkDescriptorSetLayout descset_layout,
                          VkDescriptorSet* desc_set) {

    VkDescriptorSetAllocateInfo dsai = {};
    dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.descriptorPool = dpool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &descset_layout;

    VkResult r = vkAllocateDescriptorSets(rb->dev, &dsai, desc_set);
    assert(r == VK_SUCCESS);

    VkDescriptorImageInfo imi = {};
    imi.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imi.imageView = prefftimg.view;

    VkDescriptorBufferInfo dbi_x = {};
    dbi_x.buffer = fftbuf_x.buf;
    dbi_x.offset = 0;
    dbi_x.range = sizeof(f32)*2*RCS_RESOLUTION*RCS_RESOLUTION; // 2 for complex

    VkDescriptorBufferInfo dbi_y = {};
    dbi_y.buffer = fftbuf_y.buf;
    dbi_y.offset = 0;
    dbi_y.range = sizeof(f32)*2*RCS_RESOLUTION*RCS_RESOLUTION; // 2 for complex

    VkWriteDescriptorSet img_wds = {};
    img_wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    img_wds.dstSet = *desc_set;
    img_wds.dstBinding = 0;
    img_wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    img_wds.descriptorCount = 1;
    img_wds.pImageInfo = &imi;

    VkWriteDescriptorSet xbuf_wds = {};
    xbuf_wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    xbuf_wds.dstSet = *desc_set;
    xbuf_wds.dstBinding = 1;
    xbuf_wds.dstArrayElement = 0;
    xbuf_wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    xbuf_wds.descriptorCount = 1;
    xbuf_wds.pBufferInfo = &dbi_x;
    xbuf_wds.pImageInfo = NULL;
    xbuf_wds.pTexelBufferView = NULL;

    VkWriteDescriptorSet writes[2] = {img_wds, xbuf_wds};

    vkUpdateDescriptorSets(rb->dev, 2, writes, 0, NULL);

    return false;
}

bool make_rcs_buftoimg_descset(RenderBackend* rb, VkDescriptorPool dpool,
                          Image postfftimg, Buffer fftbuf_x, Buffer fftbuf_y,
                          VkDescriptorSetLayout descset_layout,
                          VkDescriptorSet* desc_set) {

    VkDescriptorSetAllocateInfo dsai = {};
    dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.descriptorPool = dpool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &descset_layout;

    VkResult r = vkAllocateDescriptorSets(rb->dev, &dsai, desc_set);
    assert(r == VK_SUCCESS);

    VkDescriptorImageInfo imi = {};
    imi.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imi.imageView = postfftimg.view;

    VkDescriptorBufferInfo dbi_x = {};
    dbi_x.buffer = fftbuf_x.buf;
    dbi_x.offset = 0;
    dbi_x.range = sizeof(f32)*2*RCS_RESOLUTION*RCS_RESOLUTION; // 2 for complex

    VkDescriptorBufferInfo dbi_y = {};
    dbi_y.buffer = fftbuf_y.buf;
    dbi_y.offset = 0;
    dbi_y.range = sizeof(f32)*2*RCS_RESOLUTION*RCS_RESOLUTION; // 2 for complex

    VkWriteDescriptorSet img_wds = {};
    img_wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    img_wds.dstSet = *desc_set;
    img_wds.dstBinding = 0;
    img_wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    img_wds.descriptorCount = 1;
    img_wds.pImageInfo = &imi;

    VkWriteDescriptorSet xbuf_wds = {};
    xbuf_wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    xbuf_wds.dstSet = *desc_set;
    xbuf_wds.dstBinding = 1;
    xbuf_wds.dstArrayElement = 0;
    xbuf_wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    xbuf_wds.descriptorCount = 1;
    xbuf_wds.pBufferInfo = &dbi_x;
    xbuf_wds.pImageInfo = NULL;
    xbuf_wds.pTexelBufferView = NULL;


    VkWriteDescriptorSet writes[2] = {img_wds, xbuf_wds};

    vkUpdateDescriptorSets(rb->dev, 2, writes, 0, NULL);

    return false;
}