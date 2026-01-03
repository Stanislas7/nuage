$FG_GLSL_VERSION

layout(location = 0) out vec3 fragColor;

in vec2 texcoord;

uniform sampler2D hdr_tex;
uniform sampler2D gbuffer1_tex;
uniform sampler2D depth_tex;

void main()
{
    vec3 original_color = texture(hdr_tex, texcoord).rgb;

    float depth = texture(depth_tex, texcoord).r;
    // Ignore the background
    if (depth == 1.0) {
        fragColor = original_color;
        return;
    }

    float alpha = 0.0;
    // Check if we are dealing with a discarded pixel or an opaque pixel
    float test = fract(dot(gl_FragCoord.xy, vec2(0.5)));
    if (test < 0.5) {
        // This is discarded pixel. Bilinearly interpolate between the neighbors
        // to obtain an alpha value.
        alpha += textureOffset(gbuffer1_tex, texcoord, ivec2(+1,  0)).a;
        alpha += textureOffset(gbuffer1_tex, texcoord, ivec2( 0, +1)).a;
        alpha += textureOffset(gbuffer1_tex, texcoord, ivec2(-1,  0)).a;
        alpha += textureOffset(gbuffer1_tex, texcoord, ivec2( 0, -1)).a;
        alpha *= 0.25;
    } else {
        // This is an "opaque" pixel. Get the alpha value directly from it
        alpha = texture(gbuffer1_tex, texcoord).a;
    }

    // Linearly interpolate between the neighbor's color and this pixel's color
    // according to the alpha value.
    vec3 neighbor_color = textureOffset(hdr_tex, texcoord, ivec2(1, 0)).rgb;
    vec3 color = mix(original_color, neighbor_color, alpha);

    fragColor = color;
}
