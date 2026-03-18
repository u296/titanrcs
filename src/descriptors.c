#include "descriptors.h"
#include "buffers.h"
#include "cleanupstack.h"
#include "common.h"
#include "linalg.h"
#include "vulkan/vulkan_core.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    CLEANUP_START(DescriptorSetLayoutCleanup)
    {dev,*desc_layout}
    CLEANUP_END(descriptor_set_layout)

    return false;
}





bool make_uniform_buffers(const u32 n_max_inflight, VkPhysicalDevice physdev, VkDevice dev, Buffer** ubufs, void*** ubuf_mappings, Error* e_out, CleanupStack* cs) {


    * ubufs = malloc(sizeof(Buffer) * n_max_inflight);
    *ubuf_mappings = malloc(sizeof(void*) * n_max_inflight);

    for (u32 i = 0; i < n_max_inflight; i++) {
        make_buffer(physdev, dev, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &(*ubufs)[i], e_out, cs);
        vkMapMemory(dev, (*ubufs)[i].mem, 0, sizeof(UniformBufferObject), 0, &(*ubuf_mappings)[i]);
    }

    CLEANUP_START_NORES(void*)
    *ubufs
    CLEANUP_END(memfree)
    CLEANUP_START_NORES(void*)
    *ubuf_mappings
    CLEANUP_END(memfree)

    return false;
}

void update_uniformbuffer(u64 frame, VkExtent2D swp_ext,void* ubufmap) {
    UniformBufferObject u = {};

    f32 I[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };

    Mat4 imat;
    memcpy(&imat.v, I, sizeof(I));

    f32 t = (f32)frame * 1.0 / 150.0;

    f32 rotz[16] = {
        cosf(t), -sinf(t), 0, 0,
        sinf(t), cosf(t), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    f32 roty[16] = {
        cosf(t), 0, -sinf(t), 0,
        0,  1, 0, 0,
        sinf(t), 0, cosf(t), 0,
        0, 0, 0, 1
    };

    f32 transz[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, -2,
        0, 0, 0, 1
    };

    Mat4 transmat;
    memcpy(&transmat.v, transz, sizeof(I));



    f32 zfar = 10.0f;
    f32 znear = 0.01f;
    f32 fov = 120.0f * (3.1415926535f / 180.0f);

    f32 as = (f32)swp_ext.width / (f32)swp_ext.height;

    f32 f = tanf(fov/2);

    /*f32 perspective[16] = {
        1.0f / (tanf(fov/2)*as), 0, 0, 0,
        0, 1.0f / tanf(fov/2), 0, 0,
        0, 0, zfar/(zfar-znear), 1,
        0, 0, -(zfar*znear)/(zfar - znear), 0
    };*/

    f32 perspective[16] = {
        f / as, 0, 0, 0,
        0, -f, 0, 0,
        0, 0, zfar / (znear - zfar), (znear*zfar) / (znear - zfar),
        0, 0, -1, 0
    };
    Mat4 perspectivemat;
    memcpy(&perspectivemat.v, perspective, sizeof(I));

    f32 mixing = 1;//powf(sinf(0.5*t), 2.0f);

    Mat4 mixed_translation = add_m4(muls_m4(1.0f-mixing, imat), muls_m4(mixing, transmat));
    Mat4 mixed_perspective = add_m4(muls_m4(1.0f-mixing, imat), muls_m4(mixing, perspectivemat));

    

    memcpy(&u.model, roty,sizeof(I));
    memcpy(&u.view, &mixed_translation,sizeof(I));
    memcpy(&u.proj, &mixed_perspective,sizeof(I));

    u.model = transpose_m4(u.model);
    u.view = transpose_m4(u.view);
    u.proj = transpose_m4(u.proj);

    memcpy(ubufmap, &u, sizeof(u));
}

typedef struct DescriptorPoolCleanup {
    VkDevice dev;
    VkDescriptorPool dpool;
} DescriptorPoolCleanup;

void destroy_dpool(void* obj) {
    DescriptorPoolCleanup* d = (DescriptorPoolCleanup*)obj;
    vkDestroyDescriptorPool(d->dev, d->dpool, NULL);
}

bool make_descriptor_pool(const u32 n_max_inflight, VkDevice dev, VkDescriptorPool* dpool, Error* e_out, CleanupStack* cs) {
    VkDescriptorPoolSize ps = {};
    ps.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ps.descriptorCount = n_max_inflight;

    VkDescriptorPoolCreateInfo pci = {};
    pci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pci.poolSizeCount = 1;
    pci.pPoolSizes = &ps;
    pci.maxSets = n_max_inflight;

    VkResult r = vkCreateDescriptorPool(dev, &pci, NULL, dpool);
    CLEANUP_START(DescriptorPoolCleanup)
    {dev, *dpool}
    CLEANUP_END(dpool)

    VERIFY("dpool", r)

    return false;

}

bool make_descriptor_sets(const u32 n_max_inflight, VkDevice dev, VkDescriptorPool dpool, Buffer* ubufs, VkDescriptorSetLayout desc_layout, VkDescriptorSet** desc_sets, Error*e_out, CleanupStack*cs) {

    *desc_sets = malloc(sizeof(VkDescriptorSet)*n_max_inflight);

    CLEANUP_START_NORES(void*)
    *desc_sets
    CLEANUP_END(memfree)

    VkDescriptorSetLayout* desc_layout_copies = malloc(sizeof(VkDescriptorSetLayout)*n_max_inflight);
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
