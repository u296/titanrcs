#ifndef RCS_DESCSET_H
#define RCS_DESCSET_H

#include "cleanupstack.h"
#include "common.h"

bool make_rcs_dpool(VkDevice dev, VkDescriptorPool* dpool, CleanupStack* cs);

bool make_rcs_descset_layout(VkDevice dev, VkDescriptorSetLayout* desc_layout, CleanupStack* cs);

#endif