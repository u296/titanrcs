

/*
Everything that's shared by the PO and ILDC shaders goes here
*/

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_norm;
layout(location = 2) in vec3 in_edge_tangent;
layout(location = 3) in vec3 in_face_normal;
layout(location = 4) in float in_wedge_angle;

layout(location = 0) out vec4 out_prefouriertransform;
layout(location = 1) out vec4 out_phasecolor;
layout(location = 2) out vec4 out_intenscolor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 norm_proj;
    vec4 resolution_xy_L_lambda;
    vec4 cropfraction_boxsize_disablestatus_linewidth;
    vec4 infield;
} ubo;


const float pi = 3.1415926535897932384626433832795;
const float twopi = 6.283185;

vec2 cmul(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

// 1.0 is limit, Nyquist-Shannon. Lower to be safer. A "random" value might help
const float max_half_wavelens_per_pixel = 0.765;

// might want to play with this
const float undersampling_transition_len = 0.1;

float undersampling_compensation_factor_f(float lambda, float pixellen, vec3 surfnorm, vec3 toscreen) {
    float costheta = dot(surfnorm, toscreen);

    float sineshallow = pixellen / (0.5*lambda*max_half_wavelens_per_pixel);

    // undecided on this, think it can be higher for PO than ILDC
    float endrise = sineshallow + undersampling_transition_len;

    float weightfactor = smoothstep(sineshallow, endrise, costheta);

    return weightfactor;
}

float undersampling_compensation_factor_t(float lambda, float pixellen, vec3 edge_tangent, vec3 toscreen) {
    float costheta = abs(dot(normalize(edge_tangent), normalize(toscreen)));

    float sineshallow = pixellen / (0.5*lambda*max_half_wavelens_per_pixel);

    float cosshallow = sqrt(1.0 - pow(sineshallow,2));

    float weightfactor = 1.0-smoothstep(cosshallow-undersampling_transition_len, cosshallow, costheta);

    return weightfactor;
}
