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

vec2 calc_ce_cm(float wedgeangle, float phi_i) {
    float n = 1.0*wedgeangle/pi;

    // need to fix something better

    float epsilon = 0.01;

    float denom1 = (cos(pi/n) - 1.0);
    float denom2 = (cos(pi/n) - cos((2*phi_i)/n));

    denom1 = sgn(denom1) * max(abs(denom1), epsilon);
    denom2 = sgn(denom1) * max(abs(denom2), epsilon);

    if (isnan(denom1) || isinf(denom1)) {
        denom1 = 0.001;
    }
    if (isnan(denom2) || isinf(denom2)) {
        denom2 = 0.001;
    }
    

    float term1 = 1.0 / denom1;
    float term2 = 1.0 / denom2;

    float fac = sin(pi/n) / n;

    vec2 ce_cm = fac * vec2(term1 - term2, term1 + term2);

    return ce_cm;
}

void main() {

    float cropfraction = ubo.cropfraction_.x;

    float wedge_angle = in_wedge_angle;
    

    //out_prefouriertransform = cmul(reflfield, phasefactor);
    //out_phasecolor = make_color(modphase);

    vec3 pos = in_pos;
    float L = ubo.resolution_xy_L_lambda.z;
    float lambda = ubo.resolution_xy_L_lambda.w;

    vec3 edgetangent = normalize(in_edge_tangent);
    vec3 face_normal = normalize(in_face_normal);

    const float k = 2.0*pi / lambda;

    vec2 sq = pos.xy * pos.xy;

    const float moddist = pos.z + ((sq.x + sq.y) / (2.0*(L+pos.z)));
    
    const float modphase = moddist * k;

    const vec2 phasefactor = vec2(cos(modphase), sin(modphase));// phase factor to multiply before sending to fft

    vec4 infield_local = vec4(1.0, 0.0, 0.0, 0.0); // need to propagate this forward to the correct Z

    vec2 forwardphase = vec2(cos(k*pos.z), sin(k*pos.z));

    infield_local.xy = cmul(infield_local.xy, forwardphase);
    infield_local.zw = cmul(infield_local.zw, forwardphase);

    vec3 face_cotangent = cross(face_normal, edgetangent);

    vec3 local_x = face_cotangent;
    vec3 local_y = face_normal;
    vec3 local_z = edgetangent;

    vec3 indir = vec3(0.0,0.0,1.0);

    vec2 indir_local = vec2(dot(indir, local_x), dot(indir, local_y));

    float phi_i = atan(indir_local.y, indir_local.x);

    vec2 ce_cm = calc_ce_cm(wedge_angle, phi_i);

    float ce = ce_cm.x;
    float cm = ce_cm.y;

    vec3 phi_i_hat = normalize(cross(edgetangent, indir));
    vec3 beta_s_hat = normalize(cross(phi_i_hat, indir));

    vec2 einc = edgetangent.x * infield_local.xy + edgetangent.y * infield_local.zw;
    vec4 inh = vec4(-infield_local.zw, infield_local.xy);
    vec2 hinc = edgetangent.x * inh.xy + edgetangent.y * inh.zw;

    float angle_factor = max(1.0 / sqrt(1.0 - edgetangent.z), 20);

    float dl_da = angle_factor;// / (20.0/8192);

    vec4 dE = dl_da * vec4(beta_s_hat.x * ce * einc + phi_i_hat.x * cm * hinc, beta_s_hat.y * ce * einc + phi_i_hat.y * cm * hinc);

    dE /= k;
    //dE *= (20/8192);

    dE.xy = cmul(dE.xy, vec2(0,-1));
    dE.zw = cmul(dE.zw, vec2(0,-1));

    const float shiftingphase = pi * (gl_FragCoord.x + gl_FragCoord.y) / cropfraction;

    const vec2 shiftfactor = vec2(cos(shiftingphase), sin(shiftingphase));

    vec4 final = vec4(cmul(phasefactor,cmul(shiftfactor,dE.xy)),cmul(phasefactor,cmul(shiftfactor,dE.zw)));

    

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