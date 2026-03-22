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

typedef struct FramebuffersCleanup {
    VkDevice dev;
    VkFramebuffer* framebuffers;
    u32 n_framebuffers;
} FramebuffersCleanup;

void destroy_framebuffers(void* obj);

typedef struct FramebufferCleanup {
    VkDevice dev;
    VkFramebuffer framebuffers;
} FramebufferCleanup;

void destroy_framebuffer(void* obj);


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

#endif