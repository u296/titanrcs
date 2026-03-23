#version 450


layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 resolution_xy_L_;
} ubo;

void main() {
    
}