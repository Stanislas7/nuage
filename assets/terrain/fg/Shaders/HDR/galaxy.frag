$FG_GLSL_VERSION

layout(location = 0) out vec4 fragColor;

in VS_OUT {
    vec2 texcoord;
    vec3 view_vector;
} fs_in;

uniform sampler2D color_tex;

// celestial_body.glsl
vec3 celestial_body_eval_color(vec3 radiance, vec3 V);

void main()
{
    vec3 V = normalize(-fs_in.view_vector);
    vec4 texel = texture(color_tex, fs_in.texcoord);
    // fragColor = vec4(celestial_body_eval_color(radiance, V), 1.0);
    // Render nothing for now...
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
