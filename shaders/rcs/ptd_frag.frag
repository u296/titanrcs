#version 450

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

vec2 complex_dir(vec4 x) {
    vec2 lens = vec2(length(x.xy), length(x.zw));
    return normalize(lens);
}

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
    

    bool withinlims = !(psi<limit || psi>(wedge_angle-limit));

    if (withinlims) {
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
        d_cross *= pow(sin(psi/n),2);


        //d_cross = 1.0;
    } else {
        d_cross = 0.0;
    }

    vec3 vals = vec3(d_par, d_orth, d_cross);
    return vals;
}

vec2 get_psi_beta(vec3 face_normal, vec3 edge_tangent) {
    vec3 in_dir = vec3(0.0,0.0,1.0);

    vec3 face_tangent = normalize(cross(face_normal, edge_tangent)); // this should be in along the face

    // -in_dir because yeah polar coords
    vec3 indir_locbasis = vec3(dot(-in_dir, face_tangent), dot(-in_dir, normalize(face_normal)), dot(-in_dir, normalize(edge_tangent)));

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
    d_cross = sign(d_cross)*min(abs(d_cross),2);
    d_orth = sign(d_orth)*min(abs(d_orth),2);
    d_par = sign(d_par)*min(abs(d_par),2);

    vec2 out_par = E_par * d_par + E_orth * d_cross;
    vec2 out_orth = E_orth * d_orth;


    vec4 scatterfield = realv2_times_compl(e_par_out.xy, out_par)
     + realv2_times_compl(e_orth_out.xy, out_orth);


    if (!allok) {
        scatterfield = vec4(0,0,0,0);
    }

    //e_orth_in.y = 0;
    //edge_tangent.x=0;
    //return vec4(abs(d_cross),0,0,0);
    return scatterfield;
}

float undersampling_compensation_factor(float lambda, float pixellen, vec3 edge_tangent, vec3 toscreen) {
    float costheta = abs(dot(normalize(edge_tangent), normalize(toscreen)));


    // 1.0 is limit, Nyquist-Shannon. Lower to be safer.
    float maxhalfwavelens = 1.0;

    float sineshallow = pixellen / (0.5*lambda*maxhalfwavelens);

    float cosshallow = sqrt(1.0 - pow(sineshallow,2));

    // maybe want to have this higher, but ok for now
    float transitionlen = 0.05;
    float endrise = cosshallow + transitionlen;

    float weightfactor = 1.0-smoothstep(cosshallow-transitionlen, cosshallow, costheta);

    return weightfactor;
}

void main() {

    const float cropfraction = ubo.cropfraction_boxsize_disablestatus_linewidth.x;
    const float boxsize = ubo.cropfraction_boxsize_disablestatus_linewidth.y;
    const bool ILDC_disable = ubo.cropfraction_boxsize_disablestatus_linewidth.z < -0.5;
    const float linewidth = ubo.cropfraction_boxsize_disablestatus_linewidth.w;

    float wedge_angle = in_wedge_angle;

    vec3 pos = in_pos;
    vec2 resolution = ubo.resolution_xy_L_lambda.xy;
    float L = ubo.resolution_xy_L_lambda.z;
    float lambda = ubo.resolution_xy_L_lambda.w;

    vec3 edge_tangent = normalize(in_edge_tangent);
    vec3 face_normal = normalize(in_face_normal);

    const float pixellen = (boxsize / resolution.x);

    const float k = 2.0*pi / lambda;

    vec2 sq = pos.xy * pos.xy;

    const float moddist = pos.z + ((sq.x + sq.y) / (2.0*(L+pos.z)));
    
    const float modphase = moddist * k;

    const vec2 phasefactor = vec2(cos(modphase), sin(modphase));// phase factor to multiply before sending to fft

    vec4 infield_local = ubo.infield;
    //vec4(0.0, 0.0, 1.0, 0.0); // need to propagate this forward to the correct Z

    vec2 forwardphase = vec2(cos(k*pos.z), sin(k*pos.z)); 

    infield_local.xy = cmul(infield_local.xy, forwardphase);
    infield_local.zw = cmul(infield_local.zw, forwardphase);

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

    float undersample = undersampling_compensation_factor(lambda, pixellen, edge_tangent, toscreen);
    E *= undersample;

    //calc_scatterfield(k, edge_tangent, face_normal, infield_local, wedge_angle);
    vec4 tmp = E;

    // divide by jk

    E *= (1/k);

    E.zw = cmul(E.zw, vec2(0,-1));
    E.xy = cmul(E.xy, vec2(0,-1));

    const float shiftingphase = pi * (gl_FragCoord.x + gl_FragCoord.y) / cropfraction;

    const vec2 shiftfactor = vec2(cos(shiftingphase), sin(shiftingphase));

    vec4 final = vec4(cmul(phasefactor,cmul(shiftfactor,E.xy)),cmul(phasefactor,cmul(shiftfactor,E.zw))) * dl;

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