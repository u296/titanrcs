#version 450

#include "rcsfrag.glsl"

vec4 make_color(float phase) {
    phase = mod(phase, 2.0 * pi);

    return vec4(vec3(cos(phase), cos(phase + 2.0*pi/3.0), cos(phase + 4.0*pi/3.0))/1.5, 1.0);
}

float refl_fac(float cosx) {
    float h = (cosx-1)/0.3;
    h = h*h;
    return 0.01 + 0.99*exp(-h);
}

void main() {
    const vec3 norm = normalize(in_norm);
    const vec3 pos = in_pos;
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



    vec4 reflfield = infield*undersampling_compensation_factor_f(lambda,pixellen,norm,toscreen);// [V/m]

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