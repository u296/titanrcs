#include "rcs/rcs.h"
#include "cleanupstack.h"
#include "common.h"
#include "rcs/rcs_renderpass.h"



bool make_rcs_setup(RenderBackend* rb, CleanupStack* cs) {
    Error e;

    VkRenderPass renderpass;
    VkPipelineLayout rcs_pipeline_layout;
    VkPipeline rcs_pipeline;

    make_rcs_renderpass(rb, &renderpass, &e, cs);

    


    return false;
}