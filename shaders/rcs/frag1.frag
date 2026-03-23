#version 450

layout(location = 0) in vec3 pos;
//layout(location = 1) in ve

layout(location = 0) out vec2 out_prefouriertransform;
layout(location = 1) out vec4 out_phasecolor;
layout(location = 2) out vec4 out_intenscolor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 resolution_xy_L_;
} ubo;

layout(binding = 1) uniform sampler2D radar_infield;


const float pi = 3.1415926535897932384626433832795;

vec2 mul(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

vec4 make_color(float phase) {
    phase = mod(phase, 2.0 * pi);

    return vec4(cos(phase), cos(phase + 2.0*pi/3.0), cos(phase + 4.0*pi/3.0), 1.0);
}

void main() {
    const vec2 resolution = ubo.resolution_xy_L_.xy;
    const float L = ubo.resolution_xy_L_.z;

    vec2 infield = texelFetch(radar_infield, ivec2(pos.xy), 0).xy;

    const float albedo = 1.0;

    const vec2 reflfield = albedo * infield;
    const float refl_intens = length(reflfield) * length(reflfield);

    const float k = 6.18 / (25e-3);

    vec2 sq = pos.xy * pos.xy;

    const float moddist = pos.z + ((sq.x + sq.y) / (2.0*(L+pos.z)));
    
    const float modphase = moddist * k;

    const vec2 phasefactor = vec2(cos(modphase), sin(modphase));

    out_prefouriertransform = mul(reflfield, phasefactor);
    out_phasecolor = make_color(modphase);
    out_intenscolor = vec4(length(reflfield), 0.0, 0.0, 1.0);
    
}