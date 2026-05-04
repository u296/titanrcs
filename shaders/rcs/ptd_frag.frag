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
    vec4 cropfraction_;
} ubo;


const float pi = 3.1415926535897932384626433832795;

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

vec2 calc_equiv_ecurr(float k, float beta_i, float beta_s, vec3 edge_tangent, vec4 e_in, vec4 h_in) {
    float d_par = 1.0;
    float d_par_orth = 1.0; // these subs don't work
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

vec2 calc_equiv_hcurr(float k, float beta_i, float beta_s, vec3 edge_tangent, vec4 e_in, vec4 h_in) {
    float d_orth = 1.0;
    float Z0 = 377;

    vec2 hdotterm = vec2(edge_tangent.x * h_in.x + edge_tangent.y * h_in.z, edge_tangent.x*h_in.y+edge_tangent.y*h_in.w);

    vec2 hwithtwoi = vec2(cmul(hdotterm, vec2(0,2)));

    float hfactor = d_orth * Z0 / (k * sin(beta_i)*sin(beta_s));

    vec2 hwithdparkzsin2betai = cmul(hwithtwoi, vec2(hfactor,0));

    return hwithdparkzsin2betai;
}

vec4 calc_scatterfield(float k, vec3 edge_tangent, vec4 e_in) {
    float Z0 = 377.0;

    float beta_i = acos(dot(edge_tangent, vec3(0.0, 0.0, 1.0)));
    float beta_s = beta_i;

    vec4 h_in = vec4(cmul(vec2(-1.0/Z0,0), e_in.zw), cmul(vec2(1.0/Z0,0), e_in.xy));

    vec2 ie = calc_equiv_ecurr(k, beta_i, beta_s, edge_tangent, e_in, h_in);
    vec2 ih = calc_equiv_hcurr(k, beta_i, beta_s, edge_tangent, e_in, h_in);

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
    return equivfield;
}

void main() {

    float cropfraction = ubo.cropfraction_.x;

    float wedge_angle = in_wedge_angle;
    

    //out_prefouriertransform = cmul(reflfield, phasefactor);
    //out_phasecolor = make_color(modphase);

    vec3 pos = in_pos;
    float L = ubo.resolution_xy_L_lambda.z;
    float lambda = ubo.resolution_xy_L_lambda.w;

    vec3 edge_tangent = normalize(in_edge_tangent);
    vec3 face_normal = normalize(in_face_normal);

    const float k = 2.0*pi / lambda;

    vec2 sq = pos.xy * pos.xy;

    const float moddist = pos.z + ((sq.x + sq.y) / (2.0*(L+pos.z)));
    
    const float modphase = moddist * k;

    const vec2 phasefactor = vec2(cos(modphase), sin(modphase));// phase factor to multiply before sending to fft

    vec4 infield_local = vec4(0.0, 0.0, 0.0, 1.0); // need to propagate this forward to the correct Z

    vec2 forwardphase = vec2(cos(k*pos.z), sin(k*pos.z)); 

    infield_local.xy = cmul(infield_local.xy, forwardphase);
    infield_local.zw = cmul(infield_local.zw, forwardphase);

    vec3 face_cotangent = cross(face_normal, edge_tangent);

    vec3 local_x = face_cotangent;
    vec3 local_y = face_normal;
    vec3 local_z = edge_tangent;

    vec3 indir = vec3(0.0,0.0,1.0);

    float angle_factor = max(1.0 / sqrt(1.0 - edge_tangent.z), 2000);
    if (isnan(angle_factor)) {
        angle_factor = 2000;
    }

    float dl_da = angle_factor * (20.0/256.0);

    vec4 dE = dl_da * calc_scatterfield(k, edge_tangent, infield_local);

    dE *= 1.0;

    dE.xy = cmul(dE.xy, vec2(0,-1));
    dE.zw = cmul(dE.zw, vec2(0,-1));

    const float shiftingphase = pi * (gl_FragCoord.x + gl_FragCoord.y) / cropfraction;

    const vec2 shiftfactor = vec2(cos(shiftingphase), sin(shiftingphase));

    vec4 final = vec4(cmul(phasefactor,cmul(shiftfactor,dE.xy)),cmul(phasefactor,cmul(shiftfactor,dE.zw)));

    //final = dE / dl_da;

    //final = vec4(edge_tangent,0);

    final = vec4(0,0,0,0);

    out_prefouriertransform = final;


    vec4 col = vec4(1.0, 0.0, 0.0, 1.0);

    float d = dot(face_normal, vec3(0.0,0.0,-1.0));
    if (wedge_angle > 1.0*pi) {
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