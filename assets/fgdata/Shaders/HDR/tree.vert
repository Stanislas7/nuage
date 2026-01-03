$FG_GLSL_VERSION

layout(location = 0) in vec4 pos;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec4 multitexcoord0;
layout(location = 6) in vec3 instancePosition; // (x,y,z)

out VS_OUT {
    float flogz;
    vec2 texcoord;
    vec3 vs_up;
    vec3 vs_pos;
    vec3 vs_tree_pos;
    float autumn_flag;
    float scale;
} vs_out;

uniform float snow_level;
uniform float season;
uniform int num_deciduous_trees;
uniform bool use_forest_effect;
uniform float forest_effect_size;
uniform float forest_effect_shape;
uniform float WindN;
uniform float WindE;

uniform float osg_SimulationTime;
uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ModelViewProjectionMatrix;

// noise.glsl
float voronoi_noise_2d(vec2 coord, float wavelength, float xrand, float yrand);
float rand_1d(float x);

// logarithmic_depth.glsl
float logdepth_prepare_vs_depth(float z);

void main()
{
    // Throughout this code we use the instancePosition as a seed for random/noise
    // values, as it is specific to this instance of the tree.

    float num_varieties = normal.z;
    float tex_fract = floor(rand_1d(instancePosition.x) * num_varieties) / num_varieties;

    if (tex_fract < float(num_deciduous_trees) / float(num_varieties)) {
        vs_out.autumn_flag = 0.5 + fract(instancePosition.x);
    } else {
        vs_out.autumn_flag = 0.0;
    }

    tex_fract += floor(multitexcoord0.x) / num_varieties;

    // Determine a random rotation for the tree.
    float sr = sin(instancePosition.x);
    float cr = cos(instancePosition.x);
    vs_out.texcoord = vec2(tex_fract, multitexcoord0.y);
    // Determine the y texture coordinate based on whether it's summer, winter, snowy.
    vs_out.texcoord.y = vs_out.texcoord.y + 0.25 * step(snow_level, instancePosition.z) + 0.5 * season;

    // scaling
    float scale = (rand_1d(instancePosition.x) + rand_1d(instancePosition.y)) * 0.5 + 0.5;
    vec3 position = pos.xyz * normal.xxy * scale;

    // Rotation of the generic quad to specific one for the tree.
    position.xy = vec2(dot(position.xy, vec2(cr, sr)), dot(position.xy, vec2(-sr, cr)));

    // Shear by wind. Note that this only applies to the top vertices
    float instancePosition_sum = instancePosition.x + instancePosition.y + instancePosition.z;
    float wind_offset = position.z * (
        sin(osg_SimulationTime * 1.8 + instancePosition_sum * 0.01) + 1.0) * 0.0025;
    position.x = position.x + wind_offset * WindN;
    position.y = position.y + wind_offset * WindE;

    // Scale by random domains
    if (use_forest_effect) {
        float voronoi = 0.5 + 1.0 * voronoi_noise_2d(
            instancePosition.xy, forest_effect_size, forest_effect_shape, forest_effect_shape);
        position.xyz *= voronoi;
    }

    position = position + instancePosition.xyz;
    gl_Position = osg_ModelViewProjectionMatrix * vec4(position, 1.0);
    vs_out.flogz = logdepth_prepare_vs_depth(gl_Position.w);

    // Up direction in view space
    vs_out.vs_up = vec3(osg_ModelViewMatrix * vec4(0.0, 0.0, 1.0, 0.0));
    // Vertex position in view space
    vs_out.vs_pos = vec3(osg_ModelViewMatrix * vec4(position, 1.0));
    // Tree position center in view space
    vs_out.vs_tree_pos = vec3(osg_ModelViewMatrix * vec4(instancePosition.xyz, 1.0));
    // Tree radius
    vs_out.scale = normal.x * scale;
}
