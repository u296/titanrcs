#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_norm;
layout(location = 2) in vec3 in_edge_tangent;
layout(location = 3) in vec3 in_face_normal;

layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_norm;
layout(location = 2) out vec3 out_edge_tangent;
layout(location = 3) out vec3 out_face_normal;
layout(location = 4) out float out_wedge_angle; // encoded as the length of the in face normal vector

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
    mat3 normtrans = mat3(ubo.norm_trans);

    vec3 norm = normalize(normtrans * in_norm);
    vec3 edge_tangent = normalize(normtrans * in_edge_tangent);
    float wedgeangle = length(in_face_normal);
    vec3 face_normal = normalize(normtrans * in_face_normal);

    
    out_pos = mat3(ubo.model) * in_pos;
    out_norm = norm;
    out_edge_tangent = edge_tangent;
    out_face_normal = face_normal;
    out_wedge_angle = wedgeangle;

}