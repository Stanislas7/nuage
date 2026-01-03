$FG_GLSL_VERSION

uniform bool night_vision_enabled;

FG_VIEW_GLOBAL
uniform vec4 fg_Viewport[FG_NUM_VIEWS];
uniform float osg_SimulationTime;

const float FLICKER_AMOUNT = 0.05;
const float NOISE_AMOUNT = 0.05;
const float GRAIN_SIZE = 1.2;

// color.glsl
float linear_srgb_to_luminance(vec3 color);
// noise.glsl
float rand_1d(float n);
float noise_3d(vec3 x);

vec3 night_vision_apply(vec3 color, vec2 uv)
{
    if (!night_vision_enabled) {
        return color;
    }
    float lum = linear_srgb_to_luminance(color);
    float noise = noise_3d(vec3(uv * fg_Viewport[FG_VIEW_ID].zw / GRAIN_SIZE,
                                mod(osg_SimulationTime * 10000.0, 10000.0)));
    lum += (noise - 0.5) * NOISE_AMOUNT;
    float flicker = rand_1d(osg_SimulationTime) * FLICKER_AMOUNT;
    vec3 green_factor = vec3(0.1, 1.0 - FLICKER_AMOUNT + flicker, 0.2);
    color = lum * green_factor;
    // Add vignetting
    float d = distance(uv, vec2(0.5));
    color *= smoothstep(0.7, 0.2, d);
    return color;
}
