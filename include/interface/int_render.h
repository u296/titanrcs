#ifndef INT_RENDER_H
#define INT_RENDER_H
#include "common.h"
#include "context.h"

void write_interface_ubo(u64 frame, VkExtent2D swp_ext, void* ubufmap);

bool record_interface_cmdbuf(VkExtent2D swapchainextent, VkCommandBuffer cmdbuf,
                             VkPipelineLayout pipeline_layout,
                             VkPipeline pipeline, VkDescriptorSet desc_set,
                             Renderable ren, RenderContext* ctx,
                             const u32 swpch_img_i);

#endif