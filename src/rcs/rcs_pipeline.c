#include "rcs/rcs_pipeline.h"
#include "cleanupdb.h"
#include "cleanupstack.h"
#include "common.h"
#include "context.h"
#include <vulkan/vulkan_core.h>

VkResult make_shadermodule(VkDevice dev, const char* path, VkShaderModule* sm);

bool make_rcs_pipeline(RenderBackend* rb, VkExtent2D ext, VkDescriptorSetLayout descset_layout,
                       VkRenderPass renderpass, VkPipelineLayout* pipeline_layout,
                       VkPipeline* pipeline, Error* e_out, CleanupStack* cs) {

    VkShaderModule vertexshader, fragshader;

    VkResult r = make_shadermodule(rb->dev, "shaders/rcs/vert.spv", &vertexshader);
    // VERIFY("vert shader", r)

    r = make_shadermodule(rb->dev, "shaders/rcs/frag.spv", &fragshader);
    // VERIFY("frag shader", r)

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

    constexpr VkVertexInputBindingDescription rcsvertex_binding_desc = {
        0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};

    VkPipelineVertexInputStateCreateInfo vici = {};
    vici.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vici.vertexBindingDescriptionCount = 1;
    vici.pVertexBindingDescriptions = &rcsvertex_binding_desc;
    vici.vertexAttributeDescriptionCount = N_RCSVERT_ATTRIB;
    vici.pVertexAttributeDescriptions = RCSVERT_ATTRIB_DESC;

    VkPipelineInputAssemblyStateCreateInfo iaci = {};
    iaci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    iaci.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)ext.width;
    viewport.height = (float)ext.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = (struct VkOffset2D){0, 0};
    scissor.extent = ext;

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
    bas.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    bas.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo bci = {};
    bci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    bci.logicOpEnable = VK_FALSE;
    bci.logicOp = VK_LOGIC_OP_COPY;
    bci.attachmentCount = 1;
    bci.pAttachments = &bas;

    VkDynamicState dynstate[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dsci = {};
    dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dsci.dynamicStateCount = 2;
    dsci.pDynamicStates = dynstate;


    VkPipelineDepthStencilStateCreateInfo dci = {};
    dci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dci.depthTestEnable = VK_TRUE;
    dci.depthWriteEnable = VK_TRUE;
    dci.depthCompareOp = VK_COMPARE_OP_LESS;
    dci.depthBoundsTestEnable = VK_FALSE;
    dci.stencilTestEnable = VK_FALSE;


    VkPipelineLayoutCreateInfo plci = {};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &descset_layout;

    r = vkCreatePipelineLayout(rb->dev, &plci, NULL, pipeline_layout);

    CLEANUP_START(PipelineLayoutCleanup){rb->dev, *pipeline_layout} CLEANUP_END(pipelinelayout)

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
    gpci.pDepthStencilState = &dci;
    gpci.pColorBlendState = &bci;
    gpci.pDynamicState = &dsci;
    gpci.layout = *pipeline_layout;
    gpci.renderPass = renderpass;
    gpci.subpass = 0;
    gpci.basePipelineHandle = VK_NULL_HANDLE;
    gpci.basePipelineIndex = -1;

    r = vkCreateGraphicsPipelines(rb->dev, VK_NULL_HANDLE, 1, &gpci, NULL, pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, *pipeline} CLEANUP_END(pipeline)

        VERIFY("pipeline", r)

            vkDestroyShaderModule(rb->dev, vertexshader, NULL);
    vkDestroyShaderModule(rb->dev, fragshader, NULL);

    return false;
}