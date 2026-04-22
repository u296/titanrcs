#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_norm;

layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_norm;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 norm_trans;
    vec4 resolution_xy_L_;
    vec4 cropfraction_;
} ubo;

layout(push_constant) uniform PushBlock {
    float scale;
} push;

void main() {

    vec3 base_vert_pos = in_pos + (push.scale-1.0)*in_norm;

    vec4 proj_pos = ubo.proj * ubo.view * ubo.model * vec4(base_vert_pos,1.0);

    gl_Position = proj_pos;
    //out_pos = proj_pos.xyz;
    
    

    //vec3 v1 =  normalize((ubo.model * vec4(in_norm,1.0)).xyz);
    vec3 v2 = normalize(mat3(ubo.norm_trans) * in_norm);

    out_norm = v2;
    out_pos = mat3(ubo.model) * in_pos;

}