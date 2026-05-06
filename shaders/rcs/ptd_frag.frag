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
    vec4 cropfraction_boxsize_disablestatus_;
    vec4 infield;
} ubo;


const float pi = 3.1415926535897932384626433832795;
const float twopi = 6.283185;

vec3 visualize_vec2(vec2 data) {
    // 1. Calculate overall intensity (magnitude)
    float intensity = log(1.0+length(data))/1.5;
    
    // 2. Determine the ratio (0.0 = pure X, 1.0 = pure Y, 0.5 = equal)
    // Using atan2/PI provides a smoother linear distribution than x/(x+y)
    float ratio = atan(data.y, data.x) / (0.5 * pi);
    ratio = clamp(ratio, 0.0, 1.0);

    // 3. Define the Three-Channel Palette
    vec3 colorX = vec3(0.9, 0.1, 0.5); // Pink/Purple (X dominant)
    vec3 colorY = vec3(0.1, 0.8, 0.4); // Green (Y dominant)
    vec3 colorMix = vec3(0.0, 0.0, 1.0); // White (Balanced)

    // 4. Mix the colors based on ratio
    // We use a smoothstep or tent function to highlight the center "Mix"
    vec3 finalColor;
    if (ratio < 0.5) {
        // Blend from pure X to the Mix channel
        finalColor = mix(colorX, colorMix, ratio * 2.0);
    } else {
        // Blend from the Mix channel to pure Y
        finalColor = mix(colorMix, colorY, (ratio - 0.5) * 2.0);
    }

    // 5. Apply intensity to the final color
    return finalColor * intensity;
}

vec2 cmul(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

float sgn(float x) {
    if (x == 0) {
        return 1.0;
    } else {
        return sign(x);
    }
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

    float limit = 5.0 * pi / 180.0; // five deg

    //wedge_angle = 2*wedge_angle;
    

    bool withinlims = !(psi<limit || psi>(wedge_angle-limit));

    if (withinlims) {
        float term1 = cot(psi) / (1.0 - tan(0.5*pi/n)*cot((pi-psi)/n));
        float term2 = cot(psi - pi*n) / (1.0 + tan(0.5*pi/n)*cot(pi - (pi+psi)/n));

        d_cross = (2.0*cos(beta_i) / (n*sin(pi/n)))*(term1 + term2);
        //d_cross = 1.0;
    } else {
        d_cross = 0.0;
    }

    vec3 vals = vec3(d_par, d_orth, d_cross);
    return vals;
}

vec3 calc_coeffs(float wedge_angle, float psi, float beta_i, float beta_s) {
    float n = wedge_angle / pi;// n in the book

    float phi = psi + pi*(1 - 0.5*n); // for some reason they switched variables

    float alpha1 = phi;
    float alpha2 = n*pi - phi;

    const float eps = 0.0;
    float denom = 0.0;

    float D_e = 0.0;

    D_e += sin(phi/n)/(n*(cos((pi - alpha1)/n) - cos(phi/n)));
    D_e += sin(phi/n)/(n*(cos((pi - alpha2)/n) + cos(phi/n)));

    float D_m = 0.0;

    denom = sin(alpha1)*(cos((pi - alpha1)/n) - cos(phi/n));

    D_m += sin(phi)*sin((pi - alpha1)/n) / (n*denom + (denom>=0.0?eps:-eps));

    denom = sin(alpha2)*(cos((pi-alpha2)/n)+cos(phi/n));
    D_m += sin(n*pi - phi) * sin((pi-alpha2)/n) / (n*denom + (denom>=0.0?eps:-eps));

    float Q = 2 * cos(beta_i);

    float D_em = 0.0;

    denom = sin(alpha1)*(cos((pi - alpha1)/n) - cos(phi/n));
    D_em += cos(phi)*sin((pi-alpha1)/n)/(n*denom + (denom>=0.0?eps:-eps));

    denom = sin(alpha2)*(cos((pi-alpha2)/n) + cos(phi/n));
    D_em += -cos(n*pi-phi)*sin((pi-alpha2)/n)/(n*denom + (denom>=0.0?eps:-eps));

    D_em *= (Q / (sin(beta_i)+eps));

    float plusvis = psi<pi? 1.0 : 0.0;
    float minusvis = psi>(wedge_angle-pi)? 1.0 : 0.0;

    float D_par_prime = 0.0;  

    denom = cos(alpha1) + cos(phi);
    D_par_prime += -plusvis * sin(phi)/(denom + (denom>=0.0?eps:-eps));

    denom = cos(alpha2) + cos(n*pi - phi);
    D_par_prime += -minusvis * sin(n*pi - phi) / (denom + (denom>=0.0?eps:-eps));

    float D_orth_prime = 0.0;

    denom = cos(alpha1) + cos(phi);
    D_orth_prime += -plusvis * sin(phi)/(denom + (denom>=0.0?eps:-eps));

    denom = cos(alpha2) + cos(n*pi - phi);
    D_orth_prime += -minusvis * sin(n*pi - phi) / (denom + (denom>=0.0?eps:-eps));

    float D_parorth_prime = 0.0;

    denom= cos(alpha1)+cos(phi);
    D_parorth_prime += -plusvis * (Q*cos(phi)/(denom + (denom>=0.0?eps:-eps)) - cos(beta_i));

    denom = cos(alpha2)+cos(n*pi-phi);
    D_parorth_prime += +minusvis* (Q*cos(n*pi-phi)/(denom + (denom>=0.0?eps:-eps)) - cos(beta_i));

    float D_par = D_e - D_par_prime;
    float D_orth = D_m - D_orth_prime;
    float D_parorth = D_em*sin(beta_i) - D_parorth_prime;

    return vec3(D_par, D_orth, D_parorth);

}

vec2 calc_equiv_ecurr(float k, float beta_i, float beta_s, vec3 edge_tangent, vec4 e_in, vec4 h_in, vec3 coeffs) {
    float d_par = 1.0;
    float d_par_orth = 1.0; // these subs don't work

    d_par = coeffs.x;
    d_par_orth = coeffs.z;

    float Z0 = 377;

    
    vec2 dotterm = vec2(edge_tangent.x * e_in.x + edge_tangent.y * e_in.z, edge_tangent.x*e_in.y+edge_tangent.y*e_in.w);

    vec2 withtwoi = vec2(cmul(dotterm, vec2(0,2)));

    float factor = d_par / (k * Z0 * sin(beta_i)*sin(beta_i));

    vec2 withdparkzsin2betai = cmul(withtwoi, vec2(factor,0));

    vec2 hdotterm = vec2(edge_tangent.x * h_in.x + edge_tangent.y * h_in.z, edge_tangent.x*h_in.y+edge_tangent.y*h_in.w);

    vec2 hwithtwoi = vec2(cmul(hdotterm, vec2(0,2)));

    float hfactor = d_par_orth / (k * sin(beta_i));

    vec2 hwithdparkzsin2betai = cmul(hwithtwoi, vec2(hfactor,0));

    vec2 equiv_ecurr = withdparkzsin2betai + hwithdparkzsin2betai;

    //return vec2(1,1);
    return equiv_ecurr;
}

vec2 calc_equiv_hcurr(float k, float beta_i, float beta_s, vec3 edge_tangent, vec4 e_in, vec4 h_in, vec3 coeffs) {
    float d_orth = 1.0;

    d_orth = coeffs.y;

    float Z0 = 377;

    vec2 hdotterm = vec2(edge_tangent.x * h_in.x + edge_tangent.y * h_in.z, edge_tangent.x*h_in.y+edge_tangent.y*h_in.w);

    vec2 hwithtwoi = vec2(cmul(hdotterm, vec2(0,2)));

    float hfactor = d_orth * Z0 / (k * sin(beta_i)*sin(beta_s));

    vec2 hwithdparkzsin2betai = cmul(hwithtwoi, vec2(hfactor,0));

    return hwithdparkzsin2betai;
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

vec4 calc_scatterfield(float k, vec3 edge_tangent, vec3 face_normal, vec4 e_in, float wedge_angle) {
    float Z0 = 377.0;

    vec2 tmp = get_psi_beta(face_normal, edge_tangent);

    float psi_i = tmp.x;
    float psi_s = psi_i;
    float beta_i = tmp.y;
    float beta_s = pi - beta_i;    

    vec3 coeffs = calc_coeffs(wedge_angle, psi_i, beta_i, beta_s);
    /*coeffs.z = min(coeffs.z, 1.0);*/
    

    vec4 h_in = vec4(cmul(vec2(-1.0/Z0,0), e_in.zw), cmul(vec2(1.0/Z0,0), e_in.xy));

    vec2 ie = calc_equiv_ecurr(k, beta_i, beta_s, edge_tangent, e_in, h_in, coeffs);
    vec2 ih = calc_equiv_hcurr(k, beta_i, beta_s, edge_tangent, e_in, h_in, coeffs);

    vec3 scatter_dir = vec3(0,0,-1);
    vec3 hdir = normalize(cross(scatter_dir, edge_tangent));
    vec3 edir = normalize(cross(scatter_dir, hdir));
    vec2 phdir = hdir.xy; // z component guaranteed to be zero
    vec2 pedir = edir.xy;

    // multiply ie by Z0 first
    ie = cmul(ie, vec2(Z0,0));

    vec4 equivfield = vec4(phdir.x * ih.x, phdir.x*ih.y, phdir.y*ih.x, phdir.y*ih.y)
     + vec4(pedir.x*ie.x, pedir.x*ie.y, pedir.y*ie.x, pedir.y*ie.y);

    //return vec4(ih,0,0);
    //return vec4(coeffs.z,0,0,0);
    return equivfield;
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
    d_cross = sign(d_cross)*min(abs(d_cross),10);
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

void main() {

    float cropfraction = ubo.cropfraction_boxsize_disablestatus_.x;
    float boxsize = ubo.cropfraction_boxsize_disablestatus_.y;
    const bool ILDC_disable = ubo.cropfraction_boxsize_disablestatus_.z < -0.5;

    float wedge_angle = in_wedge_angle;
    // + pi; // need this for some reason
    

    //out_prefouriertransform = cmul(reflfield, phasefactor);
    //out_phasecolor = make_color(modphase);

    vec3 pos = in_pos;
    vec2 resolution = ubo.resolution_xy_L_lambda.xy;
    float L = ubo.resolution_xy_L_lambda.z;
    float lambda = ubo.resolution_xy_L_lambda.w;

    vec3 edge_tangent = normalize(in_edge_tangent);
    vec3 face_normal = normalize(in_face_normal);

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

    float x = atan(edge_tangent.y,edge_tangent.x);
    x = mod(x,pi/2);
    x -= pi/4;
    float tfac = 1.0/cos(x); // this is meh


    float mval = 1;
    float angle_factor = max(1.0 / sqrt(1.0 - edge_tangent.z*edge_tangent.z), mval);
    if (isnan(angle_factor)) {
        angle_factor = mval;
    }
    angle_factor *= x;

    float dl = (boxsize/resolution.x);// * angle_factor;

    vec4 E = calc_mitzner_scatterfield(k,edge_tangent,face_normal,infield_local,wedge_angle);
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

    float d = dot(face_normal, vec3(0.0,0.0,-1.0));
    if (wedge_angle > pi && wedge_angle < 2*pi) {
        col.g = 1;
    } else if (wedge_angle > 0.5 * pi) {
        col.b = 1;
    }

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