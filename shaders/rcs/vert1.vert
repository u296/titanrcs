#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_norm;

layout(location = 0) out vec3 out_pos;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 resolution_xy_L_;
} ubo;

void main() {
    
}