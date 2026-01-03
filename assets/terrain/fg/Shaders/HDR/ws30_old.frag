$FG_GLSL_VERSION

//////////////////////////////////////////////////////////////////
// TEST PHASE TOGGLES AND CONTROLS
//
//   Randomise texture lookups for 5 non-base textures e.g. mix_texture, detaile_texture etc.
//    Each landclass is assigned 5 random textures from the ground texture array.
//    This simulates a worst case possible texture lookup scenario, without needing access to material parameters.
//      This does not simulate multiple texture sets, of which there may be up-to 4.
//      The performance will likely be worse than in a real situation - there might be fewer textures
//        for mix, detail and other textures. This might be easier on the GPUs texture caches.
//   Possible values: 0: disabled (default),
//                    1: enabled,
//                    2: remove texture array lookups for 5 textures - only base texture + neighbour base textures
const int randomise_texture_lookups = 0;

// Constants controlling the transition from water
// to terrain depending on the terrain normal
//  A normal of 1.0 is completely horizontal.
const float WATER_START = 0.995;   //  Deeper water.
const float WATER_BEACH_TO_WATER = 0.99; //  Transition point between the shoreline and the shallow water
const float WATER_STEEP_TO_BEACH = 0.985; //  The transition point between the shoreline and the land
const float WATER_STEEP = 0.98; //  Anything with less than this value is considered not water or shoreline

//
// End of test phase controls
//////////////////////////////////////////////////////////////////

in VS_OUT {
    float flogz;
    vec2 texcoord;
    vec2 ground_texcoord;
    vec3 vertex_normal;
    vec3 vs_pos;
    vec3 ws_pos;
    vec2 raw_pos;
    vec3 rel_pos;
    float steepness;
    vec2 grad_dir;
} fs_in;

// Samplers
uniform sampler2D landclassTexture;
uniform sampler2DArray textureArray;
uniform sampler2D perlin;
// Coastline texture - generated from VPBTechnique
uniform sampler2D coastlineTexture;
uniform sampler2D waterTexture;

// Procedural texturing uniforms
uniform float eye_alt;
uniform float snowlevel;
uniform float snow_thickness_factor;
uniform float dust_cover_factor;
uniform float lichen_cover_factor;
uniform float wetness;
uniform float season;
// Wind uniforms
uniform float WindE;
uniform float WindN;
uniform int wind_effects;
// Testing code: Currently hardcoded to 2000, to allow noise functions to run while waiting for landclass lookup(s)
uniform int swatch_size;  //in metres, typically 1000 or 2000

// BEGIN Passed from VPBTechnique, not the Effect
uniform float fg_tileWidth;
uniform float fg_tileHeight;
uniform bool fg_photoScenery;
// Material parameters, from material definitions and effect defaults, for each landclass.
// xsize and ysize
uniform vec4 fg_dimensionsArray[128];
// Indices of textures in the ground texture array for different
// texture slots (grain, gradient, dot, mix, detail) for each landclass
uniform vec4 fg_textureLookup1[128];
uniform vec4 fg_textureLookup2[128];
// Each element of a vec4 contains a different materials parameter.
uniform vec4 fg_materialParams1[128];
uniform vec4 fg_materialParams2[128];
// Index into the material definition for shorelines
uniform int fg_shoreAtlasIndex;
// END Passed from VPBTechnique

uniform vec3 fg_modelOffset;
uniform float osg_SimulationTime;

// PBR material for the terrain. This is a hardcoded value applied to every
// landclass for now.
const float TERRAIN_METALLIC  = 0.0;
const float TERRAIN_ROUGHNESS = 0.95;

// ws30_util.glsl
vec4 ws30_lookup_ground_texture_array(in int texture_type,
                                      in vec2 ground_texture_coord,
                                      in int landclass_id,
                                      in vec4 dFdx_and_dFdy);
void ws30_get_landclass_id(in vec2 tile_coord,
                           in vec4 dFdx_and_dFdy,
                           out int landclass_id,
                           out ivec4 neighbor_landclass_ids,
                           out int num_unique_neighbors,
                           out vec4 mix_factor);
vec4 ws30_get_mixed_texel(in int texture_type,
                          in vec2 g_texture_coord,
                          in int landclass_id,
                          in int num_unique_neighbors,
                          in ivec4 neighbor_texel_landclass_ids,
                          in vec4 neighbor_mix_factors,
                          in vec4 dFdx_and_dFdy);
void ws30_apply_scaling(in  int  landclass,
                        in  vec2 ground_texcoord,
                        in  vec4 dxdy_gc,
                        out vec4 dxdy,
                        out vec2 st);
float get6_rand_nums(in float PRNGseed1, inout float PRNGseed2,
                     in float factor, out float[6] random_integers);
// ws30_water.glsl
vec3 ws30_perturb_water_normal(vec3 N, vec3 V, vec2 uv);
vec3 ws30_get_water_color();
// math.glsl
float pow4(float x);
// color.glsl
vec3 eotf_inverse_sRGB(vec3 srgb);
// normalmap.glsl
vec3 perturb_normal_from_height(vec3 N, vec3 V, float height);
// noise.glsl
float noise_2d(vec2 coord, float wavelength);
float noise_3d(vec3 coord, float wavelength);
float dot_noise_2d(vec2 coord, float wavelength, float fractional_max_dot_size, float d_density);
float slope_lines_2d(vec2 coord, vec2 grad_dir, float wavelength, float steepness);
// gbuffer_pack.glsl
void gbuffer_pack_pbr_opaque(vec3 normal,
                             vec3 base_color,
                             float metallic,
                             float roughness,
                             float occlusion,
                             vec3 emissive);
void gbuffer_pack_water(vec3 normal, vec3 floor_color);
// logarithmic_depth.glsl
float logdepth_encode(float z);


// A fade function for procedural scales which are smaller than a pixel
float detail_fade(float scale, float angle, float dist, float denom) {
    float fade_dist = 2000.0 * scale * angle / denom;
    return 1.0 - smoothstep(0.5 * fade_dist, fade_dist, dist);
}

void main()
{
    vec3 N = normalize(fs_in.vertex_normal);

    // For detail_fade()
    float denom = max(pow4(fs_in.steepness), 0.1);
    // distance to fragment
    float dist = length(fs_in.rel_pos);
    // altitude of fragment above sea level
    float msl_altitude = fs_in.rel_pos.z;

    vec4 texel;
    vec4 snow_texel;
    vec4 detail_texel, mix_texel, grain_texel, dot_texel, gradient_texel;

    // Scaling parameters
    vec2 st;
    vec4 dxdy;

    // Wind motion of the overlay noise simulating movement of vegetation and loose debris
    vec2 wind_pos;
    if (wind_effects > 1) {
        float wind_speed = length(vec2(WindE, WindN)) / 3.0480;
        // interfering sine wave wind pattern
        float sineTerm = sin(0.35 * wind_speed * osg_SimulationTime + 0.05 * (fs_in.raw_pos.x + fs_in.raw_pos.y));
        sineTerm = sineTerm + sin(0.3 * wind_speed * osg_SimulationTime + 0.04 * (fs_in.raw_pos.x + fs_in.raw_pos.y));
        sineTerm = sineTerm + sin(0.22 * wind_speed * osg_SimulationTime + 0.05 * (fs_in.raw_pos.x + fs_in.raw_pos.y));
        sineTerm = sineTerm / 3.0;
        // non-linear amplification to simulate gusts
        sineTerm = sineTerm * sineTerm; //smoothstep(0.2, 1.0, sineTerm);

        // wind starts moving dust and leaves at around 8 m/s
        float timeArg = 0.01 * osg_SimulationTime * wind_speed * smoothstep(8.0, 15.0, wind_speed);
        timeArg = timeArg + 0.02 * sineTerm;

        wind_pos = vec2(fs_in.raw_pos.x + WindN * timeArg, fs_in.raw_pos.y +  WindE * timeArg);
    } else {
        wind_pos = fs_in.raw_pos.xy;
    }

    // Get noise at different wavelengths in units of swatch_size
    // original assumed 4km texture.
    //
    // 5m, 5m gradient, 10m, 10m gradient: heightmap of the closeup terrain, 10m also snow
    // 50m: detail texel
    // 250m: detail texel
    // 500m: distortion and overlay
    // 1500m: overlay, detail, dust, fog
    // 2000m: overlay, detail, snow, fog

    // Perlin noise
    float noise_10m = noise_2d(fs_in.raw_pos.xy, 10.0);
    float noise_5m  = noise_2d(fs_in.raw_pos.xy,  5.0);
    float noise_2m  = noise_2d(fs_in.raw_pos.xy,  2.0);
    float noise_1m  = noise_2d(fs_in.raw_pos.xy,  1.0);
    float noise_01m = noise_2d(wind_pos.xy,       0.1);

    // Noise relative to swatch size
    float noise_25m   = noise_2d(fs_in.raw_pos.xy, swatch_size * 0.000625);
    float noise_50m   = noise_2d(fs_in.raw_pos.xy, swatch_size * 0.00125);
    float noise_250m  = noise_3d(fs_in.ws_pos.xyz, swatch_size * 0.0625);
    float noise_500m  = noise_3d(fs_in.ws_pos.xyz, swatch_size * 0.125);
    float noise_1500m = noise_3d(fs_in.ws_pos.xyz, swatch_size * 0.3750);
    float noise_2000m = noise_3d(fs_in.ws_pos.xyz, swatch_size * 0.5);
    float noise_4000m = noise_3d(fs_in.ws_pos.xyz, swatch_size);

    float dotnoisegrad_10m;

    // Slope noise
    float slopenoise_50m  = slope_lines_2d(fs_in.raw_pos.xy, fs_in.grad_dir,  50.0, fs_in.steepness);
    float slopenoise_100m = slope_lines_2d(fs_in.raw_pos.xy, fs_in.grad_dir, 100.0, fs_in.steepness);

    // Snow noise
    float snownoise_25m = mix(noise_25m, slopenoise_50m,  clamp(3.0 * (1.0 - fs_in.steepness), 0.0, 1.0));
    float snownoise_50m = mix(noise_50m, slopenoise_100m, clamp(3.0 * (1.0 - fs_in.steepness), 0.0, 1.0));


    float distortion_factor = 1.0;
    vec2 stprime;
    int flag = 1;
    int mix_flag = 1;
    float noise_term;
    float snow_alpha;

    // Tile texture coordinates range [0..1] over the tile 'rectangle'
    vec2 tile_coord = fs_in.texcoord;

    // Landclass for current fragment, and up-to 4 neighboring landclasses - 2 used currently
    int lc;
    ivec4 lc_n;

    int num_unique_neighbors = 0;

    // Mix factor of base textures for 2 neighbour landclass(es)
    vec4 mfact;

    bool water_lc = false;
    bool mix_water_texel = false;

    // Partial derivatives of s and t of ground texture coords for this fragment,
    // with respect to window (screen space) x and y axes.
    // Used to pick mipmap LoD levels, and turn off unneeded procedural detail
    // dFdx and dFdy are packed in a vec4 so multiplying everything
    // to scale takes 1 instruction slot.
    vec4 dxdy_gc = vec4(dFdx(fs_in.ground_texcoord) , dFdy(fs_in.ground_texcoord));

    ws30_get_landclass_id(tile_coord, dxdy_gc, lc, lc_n, num_unique_neighbors, mfact);
    ws30_apply_scaling(lc, fs_in.ground_texcoord, dxdy_gc, dxdy, st);

    if (fg_photoScenery) {
        // The photoscenery orthophotos are stored in the landclass texture
        // and use normalised tile coordinates
        texel = texture(landclassTexture, vec2(tile_coord.s, 1.0 - tile_coord.t));
        water_lc = (texture(waterTexture, vec2(tile_coord.s, tile_coord.t)).r > 0.1);

        // Do not attempt any mixing
        flag = 0;
        mix_flag = 0;
    } else {
        // Lookup the base texture texel for this fragment and any neighbors, with mixing
        texel = ws30_get_mixed_texel(0, fs_in.ground_texcoord, lc, num_unique_neighbors, lc_n, mfact, dxdy_gc);
        water_lc = texture(landclassTexture, vec2(tile_coord.s, tile_coord.t)).b > 0.5;
    }

    // The coastline BLUE channel provides a higher detail level for waters.
    // The coastline GREEN channel provides a steepness modified that is used
    // so that rivers and lakes are displayed with water on more angled
    // surfaces. Otherwise rivers tend to just be sand, as they flow downhill.
    float steepness_modifier = texture(coastlineTexture, tile_coord).g * 0.1;
    bool water = water_lc || texture(coastlineTexture, tile_coord).b > 0.05;
    if (water && (fs_in.steepness + steepness_modifier < WATER_START)) {
        // For water surfaces that are simply too steep to be plausible we look
        // for an adjacent landclass and mix it with a possible shoreline.
        if (water_lc) {
            if (lc == lc_n[0]) {
                // Default to the shore material definition
                lc = fg_shoreAtlasIndex;
            } else {
                lc = lc_n[0];
            }
        }

        ws30_apply_scaling(lc, fs_in.ground_texcoord, dxdy_gc, dxdy, st);

        // Look up the secondary texture
        vec4 steep_texel = ws30_lookup_ground_texture_array(0, fs_in.ground_texcoord, lc, dxdy);
        // Use the shore texture
        vec4 beach_texel = ws30_lookup_ground_texture_array(0, fs_in.ground_texcoord, fg_shoreAtlasIndex, dxdy);
        float steep_beach_factor = smoothstep(WATER_STEEP, WATER_STEEP_TO_BEACH, fs_in.steepness + steepness_modifier);
        texel = mix(steep_texel, beach_texel, steep_beach_factor);

        // Flag that we need to mix in a water texel later if appropriate.
        mix_water_texel = (fs_in.steepness + steepness_modifier > WATER_BEACH_TO_WATER);
        water = false;
    }

    if (water) {
        // This is a pure water fragment
        // Get a LOD-independent texture coordinate to sample the water textures
        const float WATER_TEXTURE_SCALE = 0.001;
        vec2 water_texcoord = fs_in.raw_pos.xy * WATER_TEXTURE_SCALE;
        N = ws30_perturb_water_normal(N, fs_in.vs_pos, water_texcoord);
        gbuffer_pack_water(N, ws30_get_water_color());
    } else {
        // Lookup material parameters for the landclass at this fragment.
        // Material parameters are from material definitions XML files (e.g. regional definitions in data/Materials/regions). They have the same names.
        // These parameters are contained in arrays of uniforms fg_materialParams1 and fg_materialParams2.
        // The uniforms are vec4s, and each parameter is mapped to a vec4 element (rgba channels).
        // In WS2 these parameters were available as uniforms of the same name.
        // Testing: The mapping is hardcoded at the moment.
        float transition_model   = fg_materialParams1[lc].r;
        float hires_overlay_bias = fg_materialParams1[lc].g;
        float grain_strength     = fg_materialParams1[lc].b;
        float intrinsic_wetness  = fg_materialParams1[lc].a;

        float dot_density     = fg_materialParams2[lc].r;
        float dot_size        = fg_materialParams2[lc].g;
        float dust_resistance = fg_materialParams2[lc].b;
        int rock_strata     = int(fg_materialParams2[lc].a);

        // dot noise
        float dotnoise_2m  = dot_noise_2d(fs_in.raw_pos.xy,  2.0 * dot_size, 0.5,  dot_density);
        float dotnoise_10m = dot_noise_2d(fs_in.raw_pos.xy, 10.0 * dot_size, 0.5,  dot_density);
        float dotnoise_15m = dot_noise_2d(fs_in.raw_pos.xy, 15.0 * dot_size, 0.33, dot_density);

        // Testing code - set randomise_texture_lookups = 2 to only look up the base texture with no extra transitions.
        detail_texel = texel;
        mix_texel = texel;
        grain_texel = texel;
        dot_texel = texel;
        gradient_texel = texel;

        // Generate 6 random numbers
        float pseed2 = 1.0;
        int tex_id_lc[6];
        float rn[6];
        if (randomise_texture_lookups == 1) {
            get6_rand_nums(float(lc)*33245.31, pseed2, 47.0, rn);
            for (int i=0;i<6;i++) tex_id_lc[i] = int(mod((float(lc)+(rn[i]*47.0)+1.0), 48.0));
        }

        if (randomise_texture_lookups == 0) {
            grain_texel    = ws30_lookup_ground_texture_array(1, st * 25.0, lc, dxdy * 25.0);
            gradient_texel = ws30_lookup_ground_texture_array(2, st *  4.0, lc, dxdy *  4.0);
        } else if (randomise_texture_lookups == 1) {
            grain_texel    = ws30_lookup_ground_texture_array(0, st * 25.0, tex_id_lc[0], dxdy * 25.0);
            gradient_texel = ws30_lookup_ground_texture_array(0, st *  4.0, tex_id_lc[1], dxdy *  4.0);
        }

        stprime = st * 80.0;
        stprime = stprime + normalize(fs_in.rel_pos).xy * 0.01 * (dotnoise_10m +  dotnoise_15m);
        vec4 dxdy_prime = vec4(dFdx(stprime), dFdy(stprime));

        if (randomise_texture_lookups == 0) {
            dot_texel = ws30_lookup_ground_texture_array(3, stprime.ts, lc, dxdy_prime.tsqp);
        } else if (randomise_texture_lookups == 1) {
            dot_texel = ws30_lookup_ground_texture_array(0, stprime.ts, tex_id_lc[2], dxdy_prime.tsqp);
        }

        // Testing: WS2 code after this, except for random texture lookups and partial derivatives

        float local_autumn_factor = texel.a;

        // we need to fade procedural structures when they get smaller than a single pixel, for this we need
        // to know under what angle we see the surface
        float view_angle = abs(dot(N, normalize(-fs_in.vs_pos)));

        // the snow texel is generated procedurally
        if (msl_altitude + 500.0 > snowlevel) {
            snow_texel = vec4(0.95, 0.95, 0.95, 1.0) * (0.9 + 0.1 * noise_500m + 0.1 * (1.0 - noise_10m));
            snow_texel.r = snow_texel.r * (0.9 + 0.05 * (noise_10m + noise_5m));
            snow_texel.g = snow_texel.g * (0.9 + 0.05 * (noise_10m + noise_5m));
            snow_texel.a = 1.0;
            noise_term = 0.1 * (noise_500m - 0.5);
            noise_term = noise_term + 0.2  * (snownoise_50m - 0.5) * detail_fade(50.0, view_angle, dist * 0.5, denom);
            noise_term = noise_term + 0.2  * (snownoise_25m - 0.5) * detail_fade(25.0, view_angle, dist * 0.5, denom);
            noise_term = noise_term + 0.3  * (    noise_10m - 0.5) * detail_fade(10.0, view_angle, dist * 0.8, denom);
            noise_term = noise_term + 0.3  * (    noise_5m  - 0.5) * detail_fade( 5.0, view_angle, dist,       denom);
            noise_term = noise_term + 0.15 * (    noise_2m  - 0.5) * detail_fade( 2.0, view_angle, dist,       denom);
            noise_term = noise_term + 0.08 * (    noise_1m  - 0.5) * detail_fade( 1.0, view_angle, dist,       denom);
            snow_texel.a = snow_texel.a * 0.2 + 0.8 * smoothstep(0.2, 0.8, 0.3 + noise_term + snow_thickness_factor + 0.0001 * (msl_altitude - snowlevel));
        }

        if (mix_flag == 1) {
            // WS2: mix_texel = texture(mix_texture, fs_in.texcoord * 1.3);
            if (randomise_texture_lookups == 0) {
                mix_texel = ws30_lookup_ground_texture_array(4, st * 1.3, lc, dxdy * 1.3);
            } else if (randomise_texture_lookups == 1)  {
                mix_texel = ws30_lookup_ground_texture_array(0, st * 1.3, tex_id_lc[3], dxdy * 1.3);
            }
            if (mix_texel.a < 0.1) {
                mix_flag = 0;
            }
        }

        // the hires overlay texture is loaded with parallax mapping
        if (flag == 1) {
            stprime = vec2(0.86*st.s + 0.5*st.t, 0.5*st.s - 0.86*st.t);
            distortion_factor = 0.97 + 0.06 * noise_500m;
            stprime = stprime * distortion_factor * 15.0;
            stprime = stprime + normalize(fs_in.rel_pos).xy * 0.022 * (noise_10m + 0.5 * noise_5m +0.25 * noise_2m - 0.875 );

            // WS2: detail_texel = texture(detail_texture, stprime); // temp

            dxdy_prime = vec4(dFdx(stprime), dFdy(stprime));
            if (randomise_texture_lookups == 0) {
                detail_texel = ws30_lookup_ground_texture_array(5, stprime , lc, dxdy_prime);
            } else if (randomise_texture_lookups == 1) {
                detail_texel = ws30_lookup_ground_texture_array(0, stprime, tex_id_lc[4], dxdy_prime);
            }
            if (detail_texel.a < 0.1) {
                flag = 0;
            }
        } // End if (flag == 1)


        // texture preparation according to detail level

        // mix in hires texture patches

        float dist_fact;
        float nSum;
        float mix_factor;

        // first the second texture overlay
        // transition model 0: random patch overlay without any gradient information
        // transition model 1: only gradient-driven transitions, no randomness

        if (mix_flag == 1) {
            nSum =  0.167 * (noise_4000m + 2.0 * noise_2000m + 2.0 * noise_1500m + noise_500m);
            nSum = mix(nSum, 0.5, max(0.0, 2.0 * (transition_model - 0.5)));
            nSum = nSum + 0.4 * (1.0 - smoothstep(0.9, 0.95, abs(fs_in.steepness)+ 0.05 * (noise_50m - 0.5))) * min(1.0, 2.0 * transition_model);
            mix_factor = smoothstep(0.5, 0.54, nSum);
            texel = mix(texel, mix_texel, mix_factor);
            local_autumn_factor = texel.a;
        }

        // then the detail texture overlay
        mix_factor = 0.0;
        if ((flag == 1) && (dist < 40000.0)) {
            dist_fact =  0.1 * smoothstep(15000.0, 40000.0, dist) - 0.03 * (1.0 - smoothstep(500.0, 5000.0, dist));
            nSum = ((1.0 - noise_2000m) + noise_1500m + 2.0 * noise_250m + noise_50m) / 5.0;
            nSum = nSum - 0.08 * (1.0 -smoothstep(0.9, 0.95, abs(fs_in.steepness)));
            mix_factor = smoothstep(0.47, 0.54, nSum + hires_overlay_bias - dist_fact);
            mix_factor = min(mix_factor, 0.8);
            texel =  mix(texel, detail_texel, mix_factor);
        }

        // rock for very steep gradients
        if (gradient_texel.a > 0.0) {
            texel = mix(texel, gradient_texel, 1.0 - smoothstep(0.75, 0.8, abs(fs_in.steepness) + 0.00002 * msl_altitude + 0.05 * (noise_50m - 0.5)));
            local_autumn_factor = texel.a;
        }

        // the dot vegetation texture overlay
        texel.rgb = mix(texel.rgb, dot_texel.rgb, dot_texel.a * (dotnoise_10m + dotnoise_15m) * detail_fade(1.0 * (dot_size * (1.0 + 0.1 * dot_size)), view_angle, dist, denom));
        texel.rgb = mix(texel.rgb, dot_texel.rgb, dot_texel.a * dotnoise_2m * detail_fade(0.1 * dot_size, view_angle, dist, denom));

        // then the grain texture overlay
        texel.rgb = mix(texel.rgb, grain_texel.rgb, grain_strength * grain_texel.a * (1.0 - mix_factor) * (1.0 - smoothstep(2000.0, 5000.0, dist)));

        // for really hires, add procedural noise overlay
        texel.rgb = texel.rgb * (1.0 + 0.4 * (noise_01m - 0.5) * detail_fade(0.1, view_angle, dist, denom));

        // autumn colors
        float autumn_factor = season * 2.0 * (1.0 - local_autumn_factor);

        texel.r = min(1.0, (1.0 + 2.5 * autumn_factor) * texel.r);
        // texel.g = texel.g;
        texel.b = max(0.0, (1.0 - 4.0 * autumn_factor) * texel.b);

        if (local_autumn_factor < 1.0) {
            float intensity = length(texel.rgb) * (1.0 - 0.5 * smoothstep(1.1, 2.0, season));
            texel.rgb = intensity * normalize(mix(texel.rgb, vec3(0.23, 0.17, 0.08), smoothstep(1.1, 2.0, season)));
        }

        // slope line overlay
        texel.rgb = texel.rgb * (1.0 - 0.12 * slopenoise_50m - 0.08 * slopenoise_100m);

        // const vec4 dust_color = vec4 (0.76, 0.71, 0.56, 1.0);
        const vec4 dust_color   = vec4 (0.76, 0.65, 0.45, 1.0);
        const vec4 lichen_color = vec4 (0.17, 0.20, 0.06, 1.0);

        // mix vegetation
        float gradient_factor = smoothstep(0.5, 1.0, fs_in.steepness);
        texel = mix(texel, lichen_color, gradient_factor * (0.4 * lichen_cover_factor +  0.8 * lichen_cover_factor * 0.5 * (noise_10m + (1.0 - noise_5m))));
        // mix dust
        texel = mix(texel, dust_color, clamp(0.5 * dust_cover_factor * dust_resistance + 3.0 * dust_cover_factor * dust_resistance * (((noise_1500m - 0.5) * 0.125) + 0.125), 0.0, 1.0));

        // mix snow
        float snow_mix_factor = 0.0;

        if (msl_altitude + 500.0 > snowlevel) {
            snow_alpha = smoothstep(0.75, 0.85, abs(fs_in.steepness));
            snow_mix_factor = snow_texel.a * smoothstep(snowlevel, snowlevel + 200.0, snow_alpha * msl_altitude + (noise_2000m + 0.1 * noise_10m - 0.55) * 400.0);
            texel = mix(texel, snow_texel, snow_mix_factor);
        }

        // get distribution of water when terrain is wet
        float combined_wetness = min(1.0, wetness + intrinsic_wetness);
        float water_threshold1;
        float water_threshold2;
        float water_factor = 0.0;

        if ((dist < 5000.0) && (combined_wetness > 0.0)) {
            water_threshold1 = 1.0 - 0.5 * combined_wetness;
            water_threshold2 = 1.0 - 0.3 * combined_wetness;
            water_factor = smoothstep(water_threshold1, water_threshold2, (0.3 * (2.0 * (1.0 - noise_10m) + (1.0 - noise_5m)) * (1.0 - smoothstep(2000.0, 5000.0, dist))) - 5.0 * (1.0 - fs_in.steepness));
        }

        // darken wet terrain
        texel.rgb = texel.rgb * (1.0 - 0.6 * combined_wetness);

        if (mix_water_texel) {
            float water_mix_factor = smoothstep(WATER_BEACH_TO_WATER, WATER_START,
                                                fs_in.steepness + steepness_modifier);
            // TODO: Actually implement this. Right now we just choose between
            // water and non-water, no in-between.
        }

        if (fg_photoScenery) {
            // Heuristic to determine the approximated height based on the
            // satellite image color.
            // - roofs (red) => high elevation
            // - grass (light green) and water (blue) => low elevation
            // - wood (dark green) => high elevation
            // - rocks, snow and concrete buildings (white) => high elevation
            float height = max(texel.r, 1.0 - texel.g - texel.b);

            // Modify the vertex normal according to the procedurally generated height
            N = perturb_normal_from_height(N, fs_in.vs_pos, height);
        }

        // This should probably be done earlier, but all of the WS20 procedural
        // texturing calculations took place in sRGB space. Just convert it to
        // linear at the end and assume everything looked great in WS20.
        texel.rgb = eotf_inverse_sRGB(texel.rgb);

        gbuffer_pack_pbr_opaque(N, texel.rgb, TERRAIN_METALLIC, TERRAIN_ROUGHNESS, 1.0, vec3(0.0));
    }

    gl_FragDepth = logdepth_encode(fs_in.flogz);
}
