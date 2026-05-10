

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
const float max_half_wavelens_per_pixel = 0.711; // could maybe compensate for 45 degree lines too by introducing some sqrt2 shit

// might want to play with this
const float undersampling_transition_angle = 5.0*pi/180.0;

float undersampling_compensation_factor_f(float lambda, float pixellen, vec3 surfnorm, vec3 toscreen) {
    float cosnu = dot(normalize(surfnorm), normalize(toscreen));

    float nu = acos(cosnu);

    float critang = atan(pixellen / (0.5*lambda*max_half_wavelens_per_pixel));

    float weightfactor;


    weightfactor = smoothstep(critang, critang+undersampling_transition_angle, pi/2-nu);

    return weightfactor;
}

float undersampling_compensation_factor_t(float lambda, float pixellen, vec3 edge_tangent, vec3 toscreen) {
    float costheta = abs(dot(normalize(edge_tangent), normalize(toscreen)));

    float theta = acos(costheta);

    float critang = atan(pixellen / (0.5*lambda*max_half_wavelens_per_pixel));

    float weightfactor = smoothstep(critang, critang+undersampling_transition_angle,theta);

    return weightfactor;
}
