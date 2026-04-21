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


vec3 make_color(float phase) {
    phase = mod(phase, 2.0 * pi);

    return vec3(cos(phase), cos(phase + 2.0*pi/3.0), cos(phase + 4.0*pi/3.0));
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

        vec3 base_col = vec3(1,1,1);
        //vec3 base_col = make_color(phase);
        vec3 norm_col = base_col / length(base_col);


        float i = log(1.0 + length(mycol.ba)) / 1.5;

        vec4 finalcol = vec4(norm_col * i, 1.0);

        float delta = 0.0;//1.0/fft_res;

        float dist_from_cent = length(newuv - vec2(0.5+delta,0.5+delta));

        float ringinner = 0.003;
        float ringthick = 0.001;

        if (dist_from_cent > ringinner && dist_from_cent < (ringinner + ringthick)) {
            finalcol.xyz = vec3(max(finalcol.x,0.5),0.0, 0.0);
        }

        outColor = finalcol;
    } else {
        vec3 mycol = texture(mytex[pc.texture_i], uv).rgb;
        outColor = vec4(mycol, 1.0);
    }
    
}