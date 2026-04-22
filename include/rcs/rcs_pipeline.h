#ifndef RCS_PIPELINE_H
#define RCS_PIPELINE_H

#include "backend/backend.h"
#include "common.h"
#include "linalg.h"

constexpr u32 N_RCSVERT_ATTRIB = 4;

/*
While normal is the normal of the vertex, as weighted by the sizes of the
triangles it's part of, the facenormal is purely from a single triangle and is
to be used with edge_tangent to form a local coordinate system.
*/
typedef struct RcsVertex {
    Vec3 pos;
    Vec3 normal;
    Vec3 edge_tangent;
    Vec3 face_normal;
} RcsVertex;

constexpr VkVertexInputAttributeDescription
    RCSVERT_ATTRIB_DESC[N_RCSVERT_ATTRIB] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(RcsVertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(RcsVertex, normal)},
        {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(RcsVertex, edge_tangent)},
        {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(RcsVertex, face_normal)}};

typedef struct RenderBackend RenderBackend;
typedef struct CleanupStack CleanupStack;

bool make_rcs_pipeline(RenderBackend* rb, VkExtent2D ext,
                       VkDescriptorSetLayout descset_layout, VkFormat* formats,
                       VkFormat* depth_format,
                       VkPipelineLayout* pipeline_layout, VkPipeline* pipeline,
                       Error* e_out, CleanupStack* cs);

bool make_ptd_pipeline(RenderBackend* rb, VkExtent2D ext,
                       VkDescriptorSetLayout descset_layout, VkFormat* formats,
                       VkFormat* depth_format, VkPipelineLayout pipeline_layout,
                       VkPipeline* pipeline, CleanupStack* cs);

bool make_rcs_reduction_pipeline(RenderBackend* rb,
                                 VkDescriptorSetLayout desc_layout,
                                 VkPipelineLayout* out_red_pipeline_layout,
                                 VkPipeline* out_red_pipeline,
                                 CleanupStack* cs);

bool make_imgtobuf_pipeline(RenderBackend* rb,
                            VkDescriptorSetLayout desc_layout,
                            VkPipelineLayout* out_imgtobuf_pipeline_layout,
                            VkPipeline* out_imgtobuf_pipeline,
                            CleanupStack* cs);

bool make_buftoimg_pipeline(RenderBackend* rb,
                            VkDescriptorSetLayout desc_layout,
                            VkPipelineLayout bufimg_transfer_pipeline_layout,
                            VkPipeline* out_buftoimg_pipeline,
                            CleanupStack* cs);

#endif