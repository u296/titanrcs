#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec2 out_prefouriertransform;
layout(location = 1) out vec4 out_phasecolor;
layout(location = 2) out vec4 out_intenscolor;

uniform sampler2D radar_infield;
uniform vec2 resolution;

void main() {
    uv = gl_Fragcoord.xy / resolution;

    

    outColor = vec4(fragColor, 1.0);
}