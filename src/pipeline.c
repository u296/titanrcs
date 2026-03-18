#include "pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cleanupstack.h"
#include "common.h"
#include "buffers.h"

void read_file(const char* filename, usize* bufsize, u8** buf) {
    

    FILE* fp = fopen(filename, "rb");

    fseek(fp, 0, SEEK_END);
    *bufsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *buf = malloc(*bufsize);

    fread(*buf, 1, *bufsize, fp);

    fclose(fp);
}

VkResult make_shadermodule(VkDevice dev, const char* path, VkShaderModule* sm) {
    usize bufsize = 0;
    u8* buf = NULL;

    read_file(path, &bufsize, &buf);

    VkShaderModuleCreateInfo vci = {};
    vci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vci.codeSize = bufsize;
    vci.pCode = (u32*)buf;
    vci.pNext = NULL;

    VkResult r = vkCreateShaderModule(dev, &vci, NULL, sm);
    free(buf);

    return r;
}

typedef struct PipelineLayoutCleanup {
    VkDevice dev;
    VkPipelineLayout layout;
} PipelineLayoutCleanup;

void destroy_pipelinelayout(void* obj) {
    PipelineLayoutCleanup* p = (PipelineLayoutCleanup*)obj;
    vkDestroyPipelineLayout(p->dev, p->layout, NULL);
}

typedef struct PipelineCleanup{
    VkDevice dev;
    VkPipeline pipeline;
} PipelineCleanup;

void destroy_pipeline(void* obj) {
    struct PipelineCleanup* p = (struct PipelineCleanup*)obj;
    vkDestroyPipeline(p->dev, p->pipeline, NULL);
}

bool make_graphicspipeline(VkDevice dev, VkExtent2D swapchainextent, VkRenderPass renderpass, VkDescriptorSetLayout desc_set_layout, VkPipelineLayout* pipeline_layout, VkPipeline* pipeline, struct Error* e_out, CleanupStack*cs) {

    VkShaderModule vertexshader, fragshader;

    VkResult r = make_shadermodule(dev, "/Users/todd/Code/diddytron/shaders/uniforms_vert.spv", &vertexshader);
    VERIFY("vert shader", r)
    
    r = make_shadermodule(dev, "/Users/todd/Code/diddytron/shaders/frag.spv", &fragshader);
    VERIFY("frag shader", r)

    VkPipelineShaderStageCreateInfo vsi = {};
    vsi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vsi.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vsi.module = vertexshader;
    vsi.pName = "main";

    VkPipelineShaderStageCreateInfo fsi = {};
    fsi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fsi.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fsi.module = fragshader;
    fsi.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vsi, fsi};

    VkPipelineVertexInputStateCreateInfo vici = {};
    vici.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vici.vertexBindingDescriptionCount = 1;
    vici.pVertexBindingDescriptions = &vertex_binding_desc;
    vici.vertexAttributeDescriptionCount = N_VERT_ATTRIB;
    vici.pVertexAttributeDescriptions = vertex_attrib_desc;

    VkPipelineInputAssemblyStateCreateInfo iaci = {};
    iaci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    iaci.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainextent.width;
    viewport.height = (float)swapchainextent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = (struct VkOffset2D){0,0};
    scissor.extent = swapchainextent;

    VkPipelineViewportStateCreateInfo vpsci = {};
    vpsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpsci.viewportCount = 1;
    vpsci.pViewports = &viewport;
    vpsci.scissorCount = 1;
    vpsci.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rci = {};
    rci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rci.depthClampEnable = VK_FALSE;
    rci.rasterizerDiscardEnable = VK_FALSE;
    rci.polygonMode = VK_POLYGON_MODE_FILL;
    rci.lineWidth = 1.0f;
    rci.cullMode = VK_CULL_MODE_NONE;
    rci.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rci.depthBiasEnable = VK_FALSE;
    rci.depthBiasConstantFactor = 0.0f;
    rci.depthBiasClamp = 0.0f;
    rci.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo msci = {};
    msci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msci.sampleShadingEnable = VK_FALSE;
    msci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msci.minSampleShading = 1.0f;
    msci.pSampleMask = NULL;
    msci.alphaToCoverageEnable = VK_FALSE;
    msci.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState bas = {};
    bas.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    bas.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo bci = {};
    bci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    bci.logicOpEnable = VK_FALSE;
    bci.logicOp = VK_LOGIC_OP_COPY;
    bci.attachmentCount = 1;
    bci.pAttachments = &bas;

    VkDynamicState dynstate[2] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dsci = {};
    dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dsci.dynamicStateCount = 2;
    dsci.pDynamicStates = dynstate;

    VkPipelineLayoutCreateInfo plci = {};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &desc_set_layout;

    
    r = vkCreatePipelineLayout(dev, &plci, NULL, pipeline_layout);

    CLEANUP_START(PipelineLayoutCleanup)
    {dev,*pipeline_layout}
    CLEANUP_END(pipelinelayout)

    VERIFY("pipeline layout", r);

    VkGraphicsPipelineCreateInfo gpci = {};
    gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpci.stageCount = 2;
    gpci.pStages = stages;
    gpci.pVertexInputState = &vici;
    gpci.pInputAssemblyState = &iaci;
    gpci.pViewportState = &vpsci;
    gpci.pRasterizationState = &rci;
    gpci.pMultisampleState = &msci;
    gpci.pDepthStencilState = NULL;
    gpci.pColorBlendState = &bci;
    gpci.pDynamicState = &dsci;
    gpci.layout = *pipeline_layout;
    gpci.renderPass = renderpass;
    gpci.subpass = 0;
    gpci.basePipelineHandle = VK_NULL_HANDLE;
    gpci.basePipelineIndex = -1;

    r = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpci, NULL, pipeline);

    CLEANUP_START(PipelineCleanup)
    {dev,*pipeline}
    CLEANUP_END(pipeline)

    VERIFY("pipeline", r)


    vkDestroyShaderModule(dev, vertexshader, NULL);
    vkDestroyShaderModule(dev, fragshader, NULL);
}


