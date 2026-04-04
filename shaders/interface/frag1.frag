#version 450

layout(push_constant) uniform pushconstants {
    uint texture_i;
} pc;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 fzoom_;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D mytex[4];

#define prefft mytex[0]

#define postfft mytex[3]

#define pi 3.14159265

vec3 make_color(float phase) {
    phase = mod(phase, 2.0 * pi);

    return vec3(cos(phase), cos(phase + 2.0*pi/3.0), cos(phase + 4.0*pi/3.0));
}

void main() {

    float zoom = 0.1;

    if (pc.texture_i == 3) {

        vec2 newuv = vec2(0.5,0.5) + (uv-vec2(0.5,0.5))*ubo.fzoom_.x;

        vec3 mycol = texture(mytex[pc.texture_i], newuv).rgb;
        // let's do the modulus in case of complex value
        float phase = atan(mycol.g, mycol.r);

        vec3 base_col = vec3(1,1,1);//make_color(phase);
        vec3 norm_col = base_col / length(base_col);

        float i = length(mycol.rg)/100000;

        //outColor = vec4(i,i,i,1.0);
        outColor = vec4(norm_col * i, 1.0);
    } else {
        vec3 mycol = texture(mytex[pc.texture_i], uv).rgb;
        outColor = vec4(mycol, 1.0);
    }
    
}