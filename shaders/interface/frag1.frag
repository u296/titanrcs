#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D mytex[4];

#define prefft mytex[0]

void main() {

    vec3 mycol = texture(prefft, uv).rgb;


    outColor = vec4(mycol, 1.0);
}