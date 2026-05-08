#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 in_norm;
layout(location = 2) in vec3 edge_tangent;
layout(location = 3) in vec3 in_face_normal;
layout(location = 4) in float wedge_angle;

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

layout(binding = 1) uniform sampler2D radar_infield;


const float pi = 3.1415926535897932384626433832795;
const float DEG_TO_RAD = 0.0174532925;//pi/180

vec2 cmul(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

vec4 make_color(float phase) {
    phase = mod(phase, 2.0 * pi);

    return vec4(vec3(cos(phase), cos(phase + 2.0*pi/3.0), cos(phase + 4.0*pi/3.0))/1.5, 1.0);
}

float refl_fac(float cosx) {
    float h = (cosx-1)/0.3;
    h = h*h;
    return 0.01 + 0.99*exp(-h);
}

float undersampling_compensation_factor(float lambda, float pixellen, vec3 surfnorm, vec3 toscreen) {
    float costheta = dot(surfnorm, toscreen);


    // 1.0 is limit, Nyquist-Shannon. Lower to be safer.
    float maxhalfwavelens = 0.9;

    float sineshallow = pixellen / (0.5*lambda*maxhalfwavelens);

    // undecided on this, think it can be higher for PO than ILDC
    float transitionlen = 0.1;
    float endrise = sineshallow + transitionlen;

    float weightfactor = smoothstep(sineshallow, endrise, costheta);

    return weightfactor;
}

void main() {
    const vec3 norm = normalize(in_norm);
    const vec2 resolution = ubo.resolution_xy_L_lambda.xy;
    const float L = ubo.resolution_xy_L_lambda.z;
    const float lambda = ubo.resolution_xy_L_lambda.w;
    const float cropfraction = ubo.cropfraction_boxsize_disablestatus_linewidth.x;
    const float boxsize = ubo.cropfraction_boxsize_disablestatus_linewidth.y;
    const bool PO_disable = ubo.cropfraction_boxsize_disablestatus_linewidth.z > 0.5;

    vec4 infield = ubo.infield;
    //vec4(1.0,0.0, 0.0,0.0);// [V/m]

    const float pixellen = (boxsize/resolution.x);

    const float albedo = 1.0;

    const vec3 toscreen = vec3(0.0, 0.0, -1.0);

    /*
    If the angle is very shallow, the pixels can't sample the amplitude quickly enough and we will get aliasing
    in the form of constructive interference where there isn't actually any.
    Since the interference will be destructive in the DC-direction anyways,
    it's better to clamp it to zero so it doesn't leak power to the front.
    */



    vec4 reflfield = infield*undersampling_compensation_factor(lambda,pixellen,norm,toscreen);// [V/m]

    const float refl_intens = length(reflfield) * length(reflfield) * refl_fac(dot(normalize(norm),toscreen));

    const float k = 2.0*pi / lambda; // [1/m]

    vec2 sq = pos.xy * pos.xy;

    const float moddist = 2.0*pos.z + ((sq.x + sq.y) / (2.0*(L+pos.z))); // [m]
    
    const float modphase = moddist * k; // [1]
    // modphase should actually have a - sign tacked on, and the FFT
    // should be an inverse FFT to strictly follow the math. But,
    // this is how it was initially setup, using a forward FFT with
    // a + sign here yields the exact same output values in theory, 
    // except that they get conjugated relative to what they should be.
    // this doesn't matter though since we're only looking at power

    const float showphase = 2*pos.z*k;

    const vec2 phasefactor = vec2(cos(modphase), sin(modphase)); // [1]

    const float shiftingphase = pi * (gl_FragCoord.x + gl_FragCoord.y) / cropfraction; // [1]

    const vec2 shiftfactor = vec2(cos(shiftingphase), sin(shiftingphase));

    float dA = pow(boxsize/resolution.x,2);

    vec4 realthing = vec4(cmul(cmul(reflfield.xy, phasefactor), shiftfactor),
      cmul(cmul(reflfield.zw, phasefactor), shiftfactor)); // V/m

    if (PO_disable) {
        realthing = vec4(0,0,0,0);
    }

    out_prefouriertransform = realthing * dA;
    out_phasecolor = make_color(modphase);
    out_intenscolor = vec4(refl_intens, refl_intens, refl_intens, 1.0);


    
/*
    out_intenscolor = vec4(
        0.5*normalize(norm)
         + vec3(0.5,0.5,0.5)
        ,1.0);// vec4(length(reflfield), 0.0, 0.0, 1.0);*/

    //out_prefouriertransform = vec2(1.0,0.0);
    //out_phasecolor = vec4(1.0,1.0,1.0,1.0); // OUTPUTS INTO ATTACHMENT 1, not 0
    
}