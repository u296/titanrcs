#version 450

layout(push_constant) uniform pushconstants {
    uint texture_i;
} pc;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D mytex[4];

#define prefft mytex[0]

#define postfft mytex[3]

void main() {

    vec3 mycol = texture(mytex[pc.texture_i], uv).rgb;


    outColor = vec4(mycol, 1.0);
}