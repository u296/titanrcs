#version 450

#include "rcsfrag.glsl"

float cot(float x) {
    return 1.0/tan(x);
}

vec3 calc_mitzner(float wedge_angle, float psi, float beta_i) {
    float plusvis = psi<pi? 1.0 : 0.0;
    float minusvis = psi>(wedge_angle-pi)? 1.0 : 0.0;
    float n = wedge_angle/pi;

    float u = -1.0 / (1.0 - cos(pi/n));
    float v = 1.0 / (cos(pi/n) + cos(pi - 2.0*psi/n));

    float corrterm = 0.5 * (plusvis * tan(psi) - minusvis * tan(psi - pi*n));

    float nsinpin = sin(pi/n)/n;

    float d_par = nsinpin * (u - v) - corrterm;
    float d_orth = nsinpin * (u + v) + corrterm;

    // todo: implement smooth clipping

    float d_cross = 0.0;

    float limit = 1.0 * pi / 180.0;

    //wedge_angle = 2*wedge_angle;
    

    

    
    float term1 = cot(psi) / (1.0 - tan(0.5*pi/n)*cot((pi-psi)/n));
    float term2 = cot(psi - pi*n) / (1.0 + tan(0.5*pi/n)*cot(pi - (pi+psi)/n));

    d_cross = (2.0*cos(beta_i) / (n*sin(pi/n)))*(term1 + term2);

    /*
    Apply sin^2 fading here, this leaves the center untouched causes a bulb like
    behavior and fades the ends to 0. Fading to 0 is physically correct since the
    electric field near the surface must be 0. This approach causes the maximum
    in d_parorth to become about half of the other constants, maximum 0.5 across all
    domains, which lines up with the expectation of the fact that a straight edge
    can't be expected to perfectly cross-polarize light.
    */
    float fade = pow(sin(psi/n),2);

    fade = exp(-1.0/(100*fade));

    /*
    It's not reasonable to clamp this value too much fuuuuuck. It must go to
    zero but it can certainly be somewhat large, energy conservation isn't a real
    thing here because these formulas are only for the backscatter case and so
    we can't really say anything about the power that goes in other directions, 
    only the back cone. At least a saving grace is that we'll never see the main
    lobe of this due to the cos beta term, but we might indeed see very powerful
    side lobes. Also since it's so sharp we need to worry about undersampling.
    */

    float b = 0.227; // maximum ~10
    fade = smoothstep(0,b,psi) * (1.0 - smoothstep(wedge_angle-b,wedge_angle,psi));

    // adaptive smooth to limit second derivative and prevent undersampling issues.
    // caps constant at 7

    b = 1.5 * (0.048 + 0.186*sqrt(cos(beta_i)));
    fade = smoothstep(0,b,psi) * (1.0 - smoothstep(wedge_angle-b,wedge_angle,psi));

    d_cross *= fade;


    
    if (isnan(d_par)) {
        d_par = 0.0;
    }
    if (isnan(d_orth)) {
        d_orth = 0.0;
    }
    if (isnan(d_cross)) {
        d_cross = 0.0;
    }
    
    d_cross = sign(d_cross)*min(abs(d_cross),7.0); // idgaf
    d_orth = sign(d_orth)*min(abs(d_orth),1.0);
    d_par = sign(d_par)*min(abs(d_par),1.0);

    vec3 vals = vec3(d_par, d_orth, d_cross);
    return vals;
}

vec2 get_psi_beta(vec3 face_normal, vec3 edge_tangent) {
    vec3 in_dir = vec3(0.0,0.0,1.0);

    vec3 face_tangent = normalize(cross(face_normal, edge_tangent)); // this should be in along the face

    vec3 minus_in_dir = -in_dir; // for some stupid reason this flips the order of d_orth and d_par, might have to do with face_tangent
    // if we do the in dir then the ray ends up inside the wedge, not good


    // -in_dir because yeah polar coords
    vec3 indir_locbasis = vec3(dot(minus_in_dir, face_tangent), dot(minus_in_dir, normalize(face_normal)), dot(minus_in_dir, normalize(edge_tangent)));

    float beta_i = mod(acos(indir_locbasis.z), twopi);
    float beta_s = pi - beta_i;

    float psi_i = mod(atan(indir_locbasis.y, indir_locbasis.x), twopi);
    float psi_s = psi_i;

    return vec2(psi_i, beta_i);
}

vec2 realv2_dot_complv2(vec2 real, vec4 comp) {
    vec2 dotprod = vec2(real.x * comp.x, real.x*comp.y) +
    vec2(real.y * comp.z, real.y*comp.w);

    return dotprod;
}

vec4 realv2_times_compl(vec2 realdir, vec2 compvalue) {
    return vec4(realdir.x * compvalue, realdir.y * compvalue);
}

vec4 calc_mitzner_scatterfield(float k, vec3 edge_tangent, vec3 face_normal, vec4 e_in, float wedge_angle) {

    vec2 tmp0 = get_psi_beta(face_normal, edge_tangent);

    float psi = tmp0.x;
    float beta = tmp0.y;

    bool allok = (psi>0.0 && psi<wedge_angle) && (0.0<beta && beta<pi);

    const float anglim = 1.0*pi/180.0;

    // discard head-on to protect from singularity, face should outweigh contrib anyway
    if (abs(psi-pi/2)<anglim || abs(psi-(wedge_angle-pi/2))<anglim) {
        allok = false;
    }

    if (beta < anglim || beta > (pi - anglim)) {
        allok = false;
    }

    vec3 indir = vec3(0,0,1);
    vec3 scatdir = vec3(0,0,-1);

    // all of these are guaranteed to have z=0
    vec3 e_orth_in = normalize(cross(edge_tangent, indir));
    vec3 e_par_in = normalize(cross(indir, e_orth_in));
    vec3 e_orth_out = normalize(cross(edge_tangent, scatdir));
    vec3 e_par_out = normalize(cross(scatdir, e_orth_out));

    bool oklengths = 
        length(e_par_in) > 0.9 && length(e_par_in) < 1.1 &&
        length(e_orth_in) > 0.9 && length(e_orth_in) < 1.1 &&
        length(e_orth_out) > 0.9 && length(e_orth_out) < 1.1 &&
        length(e_par_out) > 0.9 && length(e_par_out) < 1.1;

    allok = allok && oklengths;

    // need to compute d dot E_in

    vec3 tmp1 = calc_mitzner(wedge_angle, psi, beta);

    float d_par = tmp1.x;
    float d_orth = tmp1.y;
    float d_cross = tmp1.z;

    // first, calculate E_in in term of the local basis. These are complex values

    vec2 E_par = realv2_dot_complv2(e_par_in.xy, e_in);
    vec2 E_orth = realv2_dot_complv2(e_orth_in.xy, e_in);

    //d_cross = 0.0;
    //d_orth = 0.0;
    //d_par = 0.0;
    

    vec2 out_par = E_par * d_par + E_orth * d_cross;
    vec2 out_orth = E_orth * d_orth;


    vec4 scatterfield = realv2_times_compl(e_par_out.xy, out_par)
     + realv2_times_compl(e_orth_out.xy, out_orth);


    if (!allok) {
        scatterfield = vec4(0,0,0,0);
    }

    //return vec4(length(d_par)>0.74,0,0,0.0);
    return scatterfield;
}

void main() {

    const float cropfraction = ubo.cropfraction_boxsize_disablestatus_linewidth.x;
    const float boxsize = ubo.cropfraction_boxsize_disablestatus_linewidth.y;
    const bool ILDC_disable = ubo.cropfraction_boxsize_disablestatus_linewidth.z < -0.5;
    const float linewidth = ubo.cropfraction_boxsize_disablestatus_linewidth.w;

    float wedge_angle = in_wedge_angle;

    vec3 pos = in_pos;
    vec2 resolution = vec2(ubo.resolution_x_fftshiftyesno_L_lambda.x, ubo.resolution_x_fftshiftyesno_L_lambda.x);
    bool do_fftshift = ubo.resolution_x_fftshiftyesno_L_lambda.y > 0.5;
    float L = ubo.resolution_x_fftshiftyesno_L_lambda.z;
    float lambda = ubo.resolution_x_fftshiftyesno_L_lambda.w;

    vec3 edge_tangent = normalize(in_edge_tangent);
    vec3 face_normal = normalize(in_face_normal);

    const float pixellen = (boxsize / resolution.x);

    const float k = 2.0*pi / lambda;

    vec4 infield_local = calc_local_infield(lambda,pos.z,ubo.infield);

    vec3 face_cotangent = cross(face_normal, edge_tangent);

    vec3 local_x = face_cotangent;
    vec3 local_y = face_normal;
    vec3 local_z = edge_tangent;

    vec3 indir = vec3(0.0,0.0,1.0);
    vec3 toscreen = vec3(0.0,0.0,-1.0);

    float mval = 10; // 1 pixel can bear at most 10 pixels weight.
    // not too important to have this high, because if it's facing
    // z too much then it's not giving too much to the near-field anyway

    float angle_factor = 1.0 / max(abs(edge_tangent.x),abs(edge_tangent.y));
    angle_factor = min(angle_factor,mval);

    float dl = pixellen * angle_factor / linewidth;

    vec4 E = calc_mitzner_scatterfield(k,edge_tangent,face_normal,infield_local,wedge_angle);

    vec4 tmp = E;

    float undersample = undersampling_compensation_factor_t(lambda, pixellen, edge_tangent, toscreen);
    E *= undersample;

    //calc_scatterfield(k, edge_tangent, face_normal, infield_local, wedge_angle);

    // divide by jk

    E *= (1/k);

    E.zw = cmul(E.zw, vec2(0,-1));
    E.xy = cmul(E.xy, vec2(0,-1));


    vec4 final =  calc_prefft_value(lambda, L, pos, E) * dl;

    if (do_fftshift) {
        final = fftshift_value(cropfraction, final);
    }

    //vec4 final = vec4(cmul(phasefactor,cmul(shiftfactor,E.xy)),cmul(phasefactor,cmul(shiftfactor,E.zw))) * dl;

    //final = tmp;

    if (ILDC_disable) {
       final = vec4(0,0,0,0);
    }

    out_prefouriertransform = final;

    vec4 col = vec4(1.0, 0.0, 0.0, 1.0);

    col.g = undersample;

    //col = vec4(edge_tangent,1.0);

    //col.xyz = visualize_vec2(vec2(length(final.xy), length(final.zw)));

    out_intenscolor = col;


    
/*
    out_intenscolor = vec4(
        0.5*normalize(norm)
         + vec3(0.5,0.5,0.5)
        ,1.0);// vec4(length(reflfield), 0.0, 0.0, 1.0);*/

    //out_prefouriertransform = vec2(1.0,0.0);
    //out_phasecolor = vec4(1.0,1.0,1.0,1.0); // OUTPUTS INTO ATTACHMENT 1, not 0
    
}