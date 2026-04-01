#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D prefft;
layout(binding = 2) uniform sampler2D phase;
layout(binding = 3) uniform sampler2D intens;
layout(binding = 4) uniform sampler2D postfft;


void main() {

    vec2 mycol = texture(prefft, uv).rg;


    outColor = vec4(mycol, 0.0, 1.0);
}