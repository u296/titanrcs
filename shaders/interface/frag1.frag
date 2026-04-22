#version 450

layout(push_constant) uniform pushconstants {
    uint texture_i;
} pc;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 fzoom_fftres_;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D mytex[4];

#define prefft mytex[0]

#define postfft mytex[3]

#define pi 3.14159265

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
    vec3 colorMix = vec3(1.0, 1.0, 1.0); // White (Balanced)

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

void main() {
    float fft_res = ubo.fzoom_fftres_.y;

    float zoom = ubo.fzoom_fftres_.x;

    if (pc.texture_i == 3) {

        vec2 newuv = vec2(0.5,0.5) + (uv-vec2(0.5,0.5))*zoom;

        vec4 mycol = texture(mytex[pc.texture_i], newuv);

        mycol *= (1.0/sqrt(fft_res*fft_res)); // normalize fft for power

        // let's do the modulus in case of complex value
        float phase = 0.0;//atan(mycol.a, mycol.b);

        float xlen = length(mycol.rg);
        float ylen = length(mycol.ba);

        vec2 lens = vec2(xlen,ylen);

        vec4 finalcol = vec4(visualize_vec2(lens), 1.0);

        float delta = 0.0;//1.0/fft_res;

        float dist_from_cent = length(newuv - vec2(0.5+delta,0.5+delta));

        float ringinner = 0.03;
        float ringthick = 0.01;

        if (dist_from_cent > ringinner && dist_from_cent < (ringinner + ringthick)) {
            finalcol.xyz = vec3(max(finalcol.x,0.5),0.0, 0.0);
        }

        outColor = finalcol;
    } else {
        vec3 mycol = texture(mytex[pc.texture_i], uv).rgb;
        outColor = vec4(mycol, 1.0);
    }
    
}