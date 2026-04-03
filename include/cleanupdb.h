#ifndef CLEANUPDB_H
#define CLEANUPDB_H

#include "common.h"

typedef struct PipelineLayoutCleanup {
    VkDevice dev;
    VkPipelineLayout layout;
} PipelineLayoutCleanup;

void destroy_pipelinelayout(void* obj);

typedef struct PipelineCleanup {
    VkDevice dev;
    VkPipeline pipeline;
} PipelineCleanup;

void destroy_pipeline(void* obj);

typedef struct DescriptorSetLayoutCleanup {
    VkDevice dev;
    VkDescriptorSetLayout desc_layout;
} DescriptorSetLayoutCleanup;

void destroy_descriptor_set_layout(void* obj);

typedef struct DescriptorPoolCleanup {
    VkDevice dev;
    VkDescriptorPool dpool;
} DescriptorPoolCleanup;

void destroy_dpool(void* obj);

typedef struct MappingCleanup {
    VmaAllocator allocctx;
    VmaAllocation alloc;
} MappingCleanup;

void destroy_mapping(void* obj);

typedef struct ImageCleanup {
    Image img;
    VkDevice dev;
    VmaAllocator allocctx;
} ImageCleanup;

void destroy_image(void* obj);

#endif