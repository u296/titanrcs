#ifndef RCS_DESCSET_H
#define RCS_DESCSET_H

#include "backend/backend.h"
#include "cleanupstack.h"
#include "common.h"

bool make_rcs_dpool(VkDevice dev, VkDescriptorPool* dpool, CleanupStack* cs);

bool make_rcs_po_descset_layout(VkDevice dev,
                                VkDescriptorSetLayout* desc_layout,
                                CleanupStack* cs);

bool make_rcs_descset(RenderBackend* rb, VkDescriptorPool dpool,
                      VkDescriptorSetLayout descset_layout, Buffer ubo,
                      VkDescriptorSet* desc_set);

bool make_rcs_red_descset_layout(VkDevice dev,
                                 VkDescriptorSetLayout* desc_layout,
                                 CleanupStack* cs);

bool make_rcs_red_descset(RenderBackend* rb, VkDescriptorPool dpool,
                          Image fftimg, VkSampler sampler, Buffer extr_ssbo,
                          VkDescriptorSetLayout descset_layout,
                          VkDescriptorSet* desc_set);

bool make_rcs_imgtobuf_descset_layout(VkDevice dev,
                                      VkDescriptorSetLayout* desc_layout,
                                      CleanupStack* cs);

bool make_rcs_imgtobuf_descset(RenderBackend* rb, VkDescriptorPool dpool,
                               Image prefftimg, Buffer fftbuf_x,
                               Buffer fftbuf_y,
                               VkDescriptorSetLayout descset_layout,
                               VkDescriptorSet* desc_set);

bool make_rcs_buftoimg_descset(RenderBackend* rb, VkDescriptorPool dpool,
                               Image postfftimg, Buffer fftbuf_x,
                               Buffer fftbuf_y,
                               VkDescriptorSetLayout descset_layout,
                               VkDescriptorSet* desc_set);

#endif