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

#endif