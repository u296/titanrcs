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
    const vec2 resolution = vec2(ubo.resolution_x_fftshiftyesno_L_lambda.x, ubo.resolution_x_fftshiftyesno_L_lambda.x);
    const bool do_fftshift = ubo.resolution_x_fftshiftyesno_L_lambda.y > 0.5;
    const float L = ubo.resolution_x_fftshiftyesno_L_lambda.z;
    const float lambda = ubo.resolution_x_fftshiftyesno_L_lambda.w;
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

    vec4 loc_infield = calc_local_infield(lambda, pos.z, infield);


    vec4 reflfield = loc_infield*undersampling_compensation_factor_f(lambda,pixellen,norm,toscreen);// [V/m]

    const float refl_intens = length(reflfield) * length(reflfield) * refl_fac(dot(normalize(norm),toscreen));

    
    

    float dA = pow(boxsize/resolution.x,2);

    vec4 realthing = calc_prefft_value(lambda, L, pos, reflfield);

    if (do_fftshift) {
        realthing = fftshift_value(cropfraction, realthing);
    }

    if (PO_disable) {
        realthing = vec4(0,0,0,0);
    }

    //realthing = vec4(1,1,1,1);

    out_prefouriertransform = realthing * dA;
    out_phasecolor = make_color(calc_modphase(lambda, L, pos));
    out_intenscolor = vec4(refl_intens, refl_intens, refl_intens, 1.0);


    
/*
    out_intenscolor = vec4(
        0.5*normalize(norm)
         + vec3(0.5,0.5,0.5)
        ,1.0);// vec4(length(reflfield), 0.0, 0.0, 1.0);*/

    //out_prefouriertransform = vec2(1.0,0.0);
    //out_phasecolor = vec4(1.0,1.0,1.0,1.0); // OUTPUTS INTO ATTACHMENT 1, not 0
    
}