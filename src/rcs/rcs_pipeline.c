#include "rcs/rcs_pipeline.h"
#include "backend/backend.h"
#include "cleanupdb.h"
#include "cleanupstack.h"
#include "common.h"
#include "rcs/rcs.h"
#include "res.h"
#include <assert.h>

VkResult make_shadermodule(VkDevice dev, const char* path, VkShaderModule* sm);

bool make_rcs_pipeline(RenderBackend* rb, VkExtent2D ext,
                       VkDescriptorSetLayout descset_layout, VkFormat* formats,
                       VkFormat* depth_format,
                       VkPipelineLayout* pipeline_layout, VkPipeline* pipeline,
                       Error* e_out, CleanupStack* cs) {

    VkShaderModule vertexshader, fragshader;

    VkResult r =
        make_shadermodule(rb->dev, "shaders/rcs/rcsvert.spv", &vertexshader);
    // VERIFY("vert shader", r)

    r = make_shadermodule(rb->dev, "shaders/rcs/po.spv", &fragshader);
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
        0, sizeof(RcsVertex), VK_VERTEX_INPUT_RATE_VERTEX};

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
    bci.attachmentCount = 3;
    bci.pAttachments = (VkPipelineColorBlendAttachmentState[]){bas, bas, bas};

    VkDynamicState dynstate[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                                  VK_DYNAMIC_STATE_SCISSOR};

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

    VkPushConstantRange pcr = {};
    pcr.size = 16; // actually just need 4
    pcr.offset = 0;
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo plci = {};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &descset_layout;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &pcr;

    r = vkCreatePipelineLayout(rb->dev, &plci, NULL, pipeline_layout);

    CLEANUP_START(PipelineLayoutCleanup){rb->dev, *pipeline_layout} CLEANUP_END(
        pipelinelayout)

        VERIFY("pipeline layout", r);

    VkPipelineRenderingCreateInfo prci = {};
    prci.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    prci.colorAttachmentCount = 3;
    prci.pColorAttachmentFormats = formats;
    prci.depthAttachmentFormat = *depth_format;

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
    gpci.renderPass = VK_NULL_HANDLE; // dynamic rendering, see pnext
    gpci.subpass = 0;
    gpci.basePipelineHandle = VK_NULL_HANDLE;
    gpci.basePipelineIndex = -1;
    gpci.pNext = &prci;

    r = vkCreateGraphicsPipelines(rb->dev, VK_NULL_HANDLE, 1, &gpci, NULL,
                                  pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, *pipeline} CLEANUP_END(pipeline)

        VERIFY("pipeline", r)

            vkDestroyShaderModule(rb->dev, vertexshader, NULL);
    vkDestroyShaderModule(rb->dev, fragshader, NULL);

    return false;
}

bool make_ptd_pipeline(RenderBackend* rb, VkExtent2D ext,
                       VkDescriptorSetLayout descset_layout, VkFormat* formats,
                       VkFormat* depth_format, VkPipelineLayout pipeline_layout,
                       VkPipeline* pipeline, CleanupStack* cs) {
    VkResult r;

    VkShaderModule vert, frag;

    r = make_shadermodule(rb->dev, "shaders/rcs/rcsvert.spv", &vert);

    r = make_shadermodule(rb->dev, "shaders/rcs/ildc.spv", &frag);

    VkPipelineShaderStageCreateInfo vsi = {};
    vsi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vsi.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vsi.module = vert;
    vsi.pName = "main";

    VkPipelineShaderStageCreateInfo fsi = {};
    fsi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fsi.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fsi.module = frag;
    fsi.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vsi, fsi};

    constexpr VkVertexInputBindingDescription rcsvertex_binding_desc = {
        0, sizeof(RcsVertex), VK_VERTEX_INPUT_RATE_VERTEX};

    VkPipelineVertexInputStateCreateInfo vici = {};
    vici.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vici.vertexBindingDescriptionCount = 1;
    vici.pVertexBindingDescriptions = &rcsvertex_binding_desc;
    vici.vertexAttributeDescriptionCount = N_RCSVERT_ATTRIB;
    vici.pVertexAttributeDescriptions = RCSVERT_ATTRIB_DESC;

    VkPipelineInputAssemblyStateCreateInfo iaci = {};
    iaci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
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

    VkPipelineRasterizationLineStateCreateInfo lsci = {};
    lsci.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO;
    lsci.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_RECTANGULAR;

    VkPipelineRasterizationStateCreateInfo rci = {};
    rci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rci.depthClampEnable = VK_FALSE;
    rci.rasterizerDiscardEnable = VK_FALSE;
    rci.polygonMode = VK_POLYGON_MODE_FILL;
    rci.lineWidth = RCS_LINEWIDTH;
    rci.cullMode = VK_CULL_MODE_NONE;
    rci.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rci.depthBiasEnable = VK_FALSE;
    rci.depthBiasConstantFactor = 0.0f;
    rci.depthBiasClamp = 0.0f;
    rci.depthBiasSlopeFactor = 0.0f;
    rci.pNext = &lsci;

    VkPipelineMultisampleStateCreateInfo msci = {};
    msci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msci.sampleShadingEnable = VK_FALSE;
    msci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msci.minSampleShading = 1.0f;
    msci.pSampleMask = NULL;
    msci.alphaToCoverageEnable = VK_FALSE;
    msci.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState bas[3];
    for (u32 i = 0; i < 3; i++) {
        bas[i] = (VkPipelineColorBlendAttachmentState){};
        bas[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        bas[i].blendEnable = VK_FALSE;
    }
    bas[0].blendEnable = VK_TRUE;
    bas[0].colorBlendOp = VK_BLEND_OP_ADD;
    bas[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    bas[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    bas[0].alphaBlendOp = VK_BLEND_OP_ADD;
    bas[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    bas[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

    VkPipelineColorBlendStateCreateInfo bci = {};
    bci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    bci.logicOpEnable = VK_FALSE;
    bci.logicOp = VK_LOGIC_OP_COPY;
    bci.attachmentCount = 3;
    bci.pAttachments = bas;

    VkDynamicState dynstate[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                                  VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dsci = {};
    dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dsci.dynamicStateCount = 2;
    dsci.pDynamicStates = dynstate;

    VkPipelineDepthStencilStateCreateInfo dci = {};
    dci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dci.depthTestEnable = VK_TRUE;
    dci.depthWriteEnable = VK_FALSE; // read but don't write depth
    dci.depthCompareOp = VK_COMPARE_OP_LESS;
    dci.depthBoundsTestEnable = VK_FALSE;
    dci.stencilTestEnable = VK_FALSE;

    VkPipelineRenderingCreateInfo prci = {};
    prci.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    prci.colorAttachmentCount = 3;
    prci.pColorAttachmentFormats = formats;
    prci.depthAttachmentFormat = *depth_format;

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
    gpci.layout = pipeline_layout;
    gpci.renderPass = VK_NULL_HANDLE; // dynamic rendering, see pnext
    gpci.subpass = 0;
    gpci.basePipelineHandle = VK_NULL_HANDLE;
    gpci.basePipelineIndex = -1;
    gpci.pNext = &prci;

    r = vkCreateGraphicsPipelines(rb->dev, VK_NULL_HANDLE, 1, &gpci, NULL,
                                  pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, *pipeline} CLEANUP_END(pipeline);

    vkDestroyShaderModule(rb->dev, vert, NULL);
    vkDestroyShaderModule(rb->dev, frag, NULL);

    return false;
}

bool make_rcs_reduction_pipeline_fft(RenderBackend* rb,
                                     VkDescriptorSetLayout desc_layout,
                                     VkPipelineLayout* out_red_pipeline_layout,
                                     VkPipeline* out_red_pipeline,
                                     CleanupStack* cs) {

    VkResult r;

    VkPushConstantRange pcr = {};
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset = 0;
    pcr.size = sizeof(u32);

    VkPipelineLayoutCreateInfo plci = {};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &desc_layout;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &pcr;

    r = vkCreatePipelineLayout(rb->dev, &plci, NULL, out_red_pipeline_layout);
    CLEANUP_START(PipelineLayoutCleanup){
        rb->dev, *out_red_pipeline_layout} CLEANUP_END(pipelinelayout);

    VkShaderModule reduction_module = VK_NULL_HANDLE;
    make_shadermodule(rb->dev, "shaders/rcs/fft/reduction.spv",
                      &reduction_module);

    VkPipelineShaderStageCreateInfo sci = {};
    sci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    sci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    sci.module = reduction_module;
    sci.pName = "main";

    VkComputePipelineCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpci.stage = sci;
    cpci.basePipelineHandle = VK_NULL_HANDLE;
    cpci.basePipelineIndex = -1;
    cpci.layout = *out_red_pipeline_layout;

    r = vkCreateComputePipelines(rb->dev, VK_NULL_HANDLE, 1, &cpci, NULL,
                                 out_red_pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, *out_red_pipeline};
    CLEANUP_END(pipeline);

    vkDestroyShaderModule(rb->dev, reduction_module, NULL);

    return false;
}

bool make_rcs_reduction_pipeline_sum(RenderBackend* rb,
                                     VkDescriptorSetLayout desc_layout,
                                     VkPipelineLayout* out_red_pipeline_layout,
                                     VkPipeline* out_red_pipeline,
                                     CleanupStack* cs) {

    VkResult r;

    VkPushConstantRange pcr = {};
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset = 0;
    pcr.size = sizeof(u32);

    VkPipelineLayoutCreateInfo plci = {};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &desc_layout;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &pcr;

    r = vkCreatePipelineLayout(rb->dev, &plci, NULL, out_red_pipeline_layout);
    CLEANUP_START(PipelineLayoutCleanup){
        rb->dev, *out_red_pipeline_layout} CLEANUP_END(pipelinelayout);

    VkShaderModule reduction_module = VK_NULL_HANDLE;
    make_shadermodule(rb->dev, "shaders/rcs/sum/reduction.spv",
                      &reduction_module);

    VkPipelineShaderStageCreateInfo sci = {};
    sci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    sci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    sci.module = reduction_module;
    sci.pName = "main";

    VkComputePipelineCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpci.stage = sci;
    cpci.basePipelineHandle = VK_NULL_HANDLE;
    cpci.basePipelineIndex = -1;
    cpci.layout = *out_red_pipeline_layout;

    r = vkCreateComputePipelines(rb->dev, VK_NULL_HANDLE, 1, &cpci, NULL,
                                 out_red_pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, *out_red_pipeline};
    CLEANUP_END(pipeline);

    vkDestroyShaderModule(rb->dev, reduction_module, NULL);

    return false;
}

bool make_imgtobuf_pipeline(RenderBackend* rb,
                            VkDescriptorSetLayout desc_layout,
                            VkPipelineLayout* out_imgtobuf_pipeline_layout,
                            VkPipeline* out_imgtobuf_pipeline,
                            CleanupStack* cs) {

    VkResult r;

    /*
    this only contains the cropping factor
    */
    VkPushConstantRange pcr = {};
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset = 0;
    pcr.size = sizeof(u32);

    VkPipelineLayoutCreateInfo plci = {};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &desc_layout;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &pcr;

    r = vkCreatePipelineLayout(rb->dev, &plci, NULL,
                               out_imgtobuf_pipeline_layout);
    CLEANUP_START(PipelineLayoutCleanup){
        rb->dev, *out_imgtobuf_pipeline_layout} CLEANUP_END(pipelinelayout);

    VkShaderModule imgtobuf_module = VK_NULL_HANDLE;
    make_shadermodule(rb->dev, "shaders/rcs/fft/imgtobuf.spv",
                      &imgtobuf_module);

    VkPipelineShaderStageCreateInfo sci = {};
    sci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    sci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    sci.module = imgtobuf_module;
    sci.pName = "main";

    VkComputePipelineCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpci.stage = sci;
    cpci.basePipelineHandle = VK_NULL_HANDLE;
    cpci.basePipelineIndex = -1;
    cpci.layout = *out_imgtobuf_pipeline_layout;

    r = vkCreateComputePipelines(rb->dev, VK_NULL_HANDLE, 1, &cpci, NULL,
                                 out_imgtobuf_pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, *out_imgtobuf_pipeline};
    CLEANUP_END(pipeline);

    vkDestroyShaderModule(rb->dev, imgtobuf_module, NULL);

    return false;
}

bool make_buftoimg_pipeline(RenderBackend* rb,
                            VkDescriptorSetLayout desc_layout,
                            VkPipelineLayout bufimg_transfer_pipeline_layout,
                            VkPipeline* out_buftoimg_pipeline,
                            CleanupStack* cs) {

    VkResult r;

    VkShaderModule buftoimg_module = VK_NULL_HANDLE;
    make_shadermodule(rb->dev, "shaders/rcs/fft/buftoimg.spv",
                      &buftoimg_module);

    VkPipelineShaderStageCreateInfo sci = {};
    sci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    sci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    sci.module = buftoimg_module;
    sci.pName = "main";

    VkComputePipelineCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpci.stage = sci;
    cpci.basePipelineHandle = VK_NULL_HANDLE;
    cpci.basePipelineIndex = -1;
    cpci.layout = bufimg_transfer_pipeline_layout;

    r = vkCreateComputePipelines(rb->dev, VK_NULL_HANDLE, 1, &cpci, NULL,
                                 out_buftoimg_pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, *out_buftoimg_pipeline};
    CLEANUP_END(pipeline);

    vkDestroyShaderModule(rb->dev, buftoimg_module, NULL);

    return false;
}

typedef struct DownscaleSpecConstants {
    u32 wg_size_x;
    u32 wg_size_y;
    u32 use_loop;
    u32 perthread_pixels_x;
    u32 perthread_pixels_y;
} DownscaleSpecConstants;

void verify_specconst(u32 target_scaling, DownscaleSpecConstants s) {
    bool scalingcorrect =
        s.wg_size_x * s.perthread_pixels_x == target_scaling &&
        s.wg_size_y * s.perthread_pixels_y == target_scaling;
    bool workgrouplimits = s.wg_size_x * s.wg_size_y <= 1024;

    if (!scalingcorrect) {
        printf("INCORRECT SCALING: CHECK SCALING FOR %u\n", target_scaling);
        abort();
    }
    if (!workgrouplimits) {
        printf("MAX WORKGROUP SIZE OF 1024 EXCEEDED FOR SCALING %u\n",
               target_scaling);
        abort();
    }
}

bool make_downscale_pipelines(RenderBackend* rb,
                              VkDescriptorSetLayout desc_layout,
                              DownscaleResources* out_downscaleraes,
                              CleanupStack* cs) {

    VkResult r;

    VkPipelineLayout pl = VK_NULL_HANDLE;

    DownscaleResources dres = {};

    /*
    this only contains the cropping factor
    */
    VkPushConstantRange pcr = {};
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset = 0;
    pcr.size = sizeof(u32);

    VkPipelineLayoutCreateInfo plci = {};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &desc_layout;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &pcr;

    r = vkCreatePipelineLayout(rb->dev, &plci, NULL, &pl);
    CLEANUP_START(PipelineLayoutCleanup){rb->dev,
                                         pl} CLEANUP_END(pipelinelayout);

    VkShaderModule downscale_module = VK_NULL_HANDLE;
    make_shadermodule(rb->dev, "shaders/rcs/sum/downscalesum.spv",
                      &downscale_module);

    VkSpecializationMapEntry spec_wgx = {};
    spec_wgx.constantID = 0;
    spec_wgx.offset = offsetof(DownscaleSpecConstants, wg_size_x);
    spec_wgx.size = sizeof(((DownscaleSpecConstants*)0)->wg_size_x);

    VkSpecializationMapEntry spec_wgy = {};
    spec_wgy.constantID = 1;
    spec_wgy.offset = offsetof(DownscaleSpecConstants, wg_size_y);
    spec_wgy.size = sizeof(((DownscaleSpecConstants*)0)->wg_size_y);

    VkSpecializationMapEntry spec_useloop = {};
    spec_useloop.constantID = 2;
    spec_useloop.offset = offsetof(DownscaleSpecConstants, use_loop);
    spec_useloop.size = sizeof(((DownscaleSpecConstants*)0)->use_loop);

    VkSpecializationMapEntry spec_perthread_pixels_x = {};
    spec_perthread_pixels_x.constantID = 3;
    spec_perthread_pixels_x.offset = offsetof(DownscaleSpecConstants, perthread_pixels_x);
    spec_perthread_pixels_x.size =
        sizeof(((DownscaleSpecConstants*)0)->perthread_pixels_x);

    VkSpecializationMapEntry spec_perthread_pixels_y = {};
    spec_perthread_pixels_y.constantID = 4;
    spec_perthread_pixels_y.offset = offsetof(DownscaleSpecConstants, perthread_pixels_y);
    spec_perthread_pixels_y.size =
        sizeof(((DownscaleSpecConstants*)0)->perthread_pixels_y);

#define N_SPEC_ENTRIES 5

    VkSpecializationMapEntry spec_entries[N_SPEC_ENTRIES] = {
        spec_wgx, spec_wgy, spec_useloop, spec_perthread_pixels_x,
        spec_perthread_pixels_y};

    DownscaleSpecConstants spec8_consts = {
        .wg_size_x = 8 / 1,
        .wg_size_y = 8 / 1,
        .use_loop = 0,
        .perthread_pixels_x = 1,
        .perthread_pixels_y = 1,
    };
    dres.downscale8.scaling = 8;

    DownscaleSpecConstants spec16_consts = {.wg_size_x = 16 / 4,
                                            .wg_size_y = 16 / 1,
                                            .use_loop = 0,
                                            .perthread_pixels_x = 4,
                                            .perthread_pixels_y = 1};

    dres.downscale16.scaling = 16;

    DownscaleSpecConstants spec32_consts = {.wg_size_x = 32 / 8,
                                            .wg_size_y = 32 / 1,
                                            .use_loop = 0,
                                            .perthread_pixels_x = 8,
                                            .perthread_pixels_y = 1};

    dres.downscale32.scaling = 32;

    DownscaleSpecConstants spec64_consts = {.wg_size_x = 64 / 4,
                                            .wg_size_y = 64 / 1,
                                            .use_loop = 0,
                                            .perthread_pixels_x = 4,
                                            .perthread_pixels_y = 1};

    dres.downscale64.scaling = 64;

    verify_specconst(8, spec8_consts);
    verify_specconst(16, spec16_consts);
    verify_specconst(32, spec32_consts);
    verify_specconst(64, spec64_consts);

    VkSpecializationInfo spec8 = {.mapEntryCount = N_SPEC_ENTRIES,
                                  .pMapEntries = spec_entries,
                                  .dataSize = sizeof(DownscaleSpecConstants),
                                  .pData = &spec8_consts};

    VkSpecializationInfo spec16 = {.mapEntryCount = N_SPEC_ENTRIES,
                                   .pMapEntries = spec_entries,
                                   .dataSize = sizeof(DownscaleSpecConstants),
                                   .pData = &spec16_consts};

    VkSpecializationInfo spec32 = {.mapEntryCount = N_SPEC_ENTRIES,
                                   .pMapEntries = spec_entries,
                                   .dataSize = sizeof(DownscaleSpecConstants),
                                   .pData = &spec32_consts};

    VkSpecializationInfo spec64 = {.mapEntryCount = N_SPEC_ENTRIES,
                                   .pMapEntries = spec_entries,
                                   .dataSize = sizeof(DownscaleSpecConstants),
                                   .pData = &spec64_consts};

    VkPipelineShaderStageCreateInfo sci8 = {};
    sci8.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    sci8.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    sci8.module = downscale_module;
    sci8.pName = "main";
    sci8.pSpecializationInfo = &spec8;

    VkPipelineShaderStageCreateInfo sci16 = {};
    sci16.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    sci16.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    sci16.module = downscale_module;
    sci16.pName = "main";
    sci16.pSpecializationInfo = &spec16;

    VkPipelineShaderStageCreateInfo sci32 = {};
    sci32.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    sci32.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    sci32.module = downscale_module;
    sci32.pName = "main";
    sci32.pSpecializationInfo = &spec32;

    VkPipelineShaderStageCreateInfo sci64 = {};
    sci64.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    sci64.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    sci64.module = downscale_module;
    sci64.pName = "main";
    sci64.pSpecializationInfo = &spec64;

    VkComputePipelineCreateInfo cpci8 = {};
    cpci8.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpci8.stage = sci8;
    cpci8.basePipelineHandle = VK_NULL_HANDLE;
    cpci8.basePipelineIndex = -1;
    cpci8.layout = pl;

    VkComputePipelineCreateInfo cpci16 = {};
    cpci16.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpci16.stage = sci16;
    cpci16.basePipelineHandle = VK_NULL_HANDLE;
    cpci16.basePipelineIndex = -1;
    cpci16.layout = pl;

    VkComputePipelineCreateInfo cpci32 = {};
    cpci32.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpci32.stage = sci32;
    cpci32.basePipelineHandle = VK_NULL_HANDLE;
    cpci32.basePipelineIndex = -1;
    cpci32.layout = pl;

    VkComputePipelineCreateInfo cpci64 = {};
    cpci64.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpci64.stage = sci64;
    cpci64.basePipelineHandle = VK_NULL_HANDLE;
    cpci64.basePipelineIndex = -1;
    cpci64.layout = pl;

    r = vkCreateComputePipelines(rb->dev, VK_NULL_HANDLE, 1, &cpci8, NULL,
                                 &dres.downscale8.pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, dres.downscale8.pipeline};
    CLEANUP_END(pipeline);

    r = vkCreateComputePipelines(rb->dev, VK_NULL_HANDLE, 1, &cpci16, NULL,
                                 &dres.downscale16.pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, dres.downscale16.pipeline};
    CLEANUP_END(pipeline);

    r = vkCreateComputePipelines(rb->dev, VK_NULL_HANDLE, 1, &cpci32, NULL,
                                 &dres.downscale32.pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, dres.downscale32.pipeline};
    CLEANUP_END(pipeline);

    r = vkCreateComputePipelines(rb->dev, VK_NULL_HANDLE, 1, &cpci64, NULL,
                                 &dres.downscale64.pipeline);

    CLEANUP_START(PipelineCleanup){rb->dev, dres.downscale64.pipeline};
    CLEANUP_END(pipeline);

    dres.downscale_pipeline_layout = pl;

    *out_downscaleraes = dres;

    vkDestroyShaderModule(rb->dev, downscale_module, NULL);

    return false;
}