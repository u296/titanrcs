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

#endif