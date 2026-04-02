#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_norm;

layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_norm;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat3 norm_trans;
    vec4 resolution_xy_L_;
} ubo;

void main() {

    vec4 proj_pos = ubo.proj * ubo.model * vec4(in_pos, 1.0);

    gl_Position = proj_pos;
    out_pos = proj_pos.xyz;
    out_norm = ubo.norm_trans * in_norm;
}