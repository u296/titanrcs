#ifndef PIPELINE_H
#define PIPELINE_H
#include "cleanupstack.h"
#include "common.h"

bool make_graphicspipeline(VkDevice dev, VkExtent2D swapchainextent, VkRenderPass renderpass, VkDescriptorSetLayout desc_set_layout, VkPipelineLayout* pipeline_layout, VkPipeline* pipeline, struct Error* e_out,CleanupStack*cs);


#endif