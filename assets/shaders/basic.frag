#version 330 core
in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPos;
out vec4 FragColor;

uniform vec3 uLightDir = vec3(0.0, 1.0, 0.0);
uniform vec3 uLightColor = vec3(1.0, 0.95, 0.9);
uniform vec3 uAmbientColor = vec3(0.25, 0.28, 0.32);
uniform vec3 uColor = vec3(1.0, 1.0, 1.0);
uniform bool uUseUniformColor = false;
uniform bool uTerrainShading = false;
uniform vec3 uCameraPos = vec3(0.0, 0.0, 0.0);
uniform vec3 uTerrainFogColor = vec3(0.72, 0.82, 0.94);
uniform float uTerrainHeightMin = 0.0;
uniform float uTerrainHeightMax = 1500.0;
uniform float uTerrainNoiseScale = 0.002;
uniform float uTerrainNoiseStrength = 0.3;
uniform float uTerrainSlopeStart = 0.3;
uniform float uTerrainSlopeEnd = 0.7;
uniform float uTerrainSlopeDarken = 0.3;
uniform float uTerrainFogDistance = 12000.0;
uniform float uTerrainDesaturate = 0.2;
uniform vec3 uTerrainTint = vec3(0.45, 0.52, 0.33);
uniform float uTerrainTintStrength = 0.15;
uniform float uTerrainDistanceDesatStart = 3000.0;
uniform float uTerrainDistanceDesatEnd = 12000.0;
uniform float uTerrainDistanceDesatStrength = 0.35;
uniform float uTerrainDistanceContrastLoss = 0.25;
uniform bool uTerrainUseTextures = false;
uniform bool uTerrainUseMasks = false;
uniform sampler2D uTerrainTexGrass;
uniform sampler2D uTerrainTexGrassB;
uniform sampler2D uTerrainTexForest;
uniform sampler2D uTerrainTexRock;
uniform sampler2D uTerrainTexDirt;
uniform sampler2D uTerrainTexUrban;
uniform sampler2D uTerrainTexGrassNormal;
uniform sampler2D uTerrainTexDirtNormal;
uniform sampler2D uTerrainTexRockNormal;
uniform sampler2D uTerrainTexUrbanNormal;
uniform sampler2D uTerrainTexGrassRough;
uniform sampler2D uTerrainTexDirtRough;
uniform sampler2D uTerrainTexRockRough;
uniform sampler2D uTerrainTexUrbanRough;
uniform float uTerrainTexScale = 0.02;
uniform float uTerrainDetailScale = 0.08;
uniform float uTerrainDetailStrength = 0.35;
uniform float uTerrainRockSlopeStart = 0.35;
uniform float uTerrainRockSlopeEnd = 0.7;
uniform float uTerrainRockStrength = 0.7;
uniform float uTerrainMacroScale = 0.0012;
uniform float uTerrainMacroStrength = 0.25;
uniform float uTerrainMegaScale = 0.00035;
uniform float uTerrainMegaStrength = 0.12;
uniform float uTerrainFarmlandStrength = 0.5;
uniform float uTerrainFarmlandStripeScale = 0.004;
uniform float uTerrainFarmlandStripeContrast = 0.6;
uniform float uTerrainScrubStrength = 0.25;
uniform float uTerrainScrubNoiseScale = 0.0016;
uniform vec3 uTerrainGrassTintA = vec3(0.75, 0.95, 0.65);
uniform vec3 uTerrainGrassTintB = vec3(0.55, 0.7, 0.45);
uniform float uTerrainGrassTintStrength = 0.35;
uniform vec3 uTerrainForestTintA = vec3(0.7, 0.85, 0.6);
uniform vec3 uTerrainForestTintB = vec3(0.5, 0.65, 0.45);
uniform float uTerrainForestTintStrength = 0.25;
uniform vec3 uTerrainUrbanTintA = vec3(0.95, 0.95, 0.95);
uniform vec3 uTerrainUrbanTintB = vec3(0.75, 0.78, 0.8);
uniform float uTerrainUrbanTintStrength = 0.2;
uniform float uTerrainMicroScale = 0.22;
uniform float uTerrainMicroStrength = 0.18;
uniform float uTerrainWaterDetailScale = 0.08;
uniform float uTerrainWaterDetailStrength = 0.25;
uniform vec3 uTerrainWaterColor = vec3(0.14, 0.32, 0.55);
uniform float uTerrainShoreWidth = 0.45;
uniform float uTerrainShoreFeather = 0.18;
uniform float uTerrainWetStrength = 0.35;
uniform float uTerrainFarmTexScale = 0.12;
uniform bool uTerrainHasMaskTex = false;
uniform sampler2D uTerrainMaskTex;
uniform vec2 uTerrainMaskOrigin;
uniform vec2 uTerrainMaskInvSize;
uniform float uTerrainMaskFeatherMeters = 42.0;
uniform float uTerrainMaskJitterMeters = 18.0;
uniform float uTerrainMaskEdgeNoise = 0.35;
uniform bool uTerrainDebugMaskView = false;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

vec2 rotate90(vec2 uv, float pick) {
    if (pick < 1.0) {
        return uv;
    }
    if (pick < 2.0) {
        return vec2(uv.y, -uv.x);
    }
    if (pick < 3.0) {
        return vec2(-uv.x, -uv.y);
    }
    return vec2(-uv.y, uv.x);
}

int maskClassAt(vec2 worldPos) {
    vec2 maskUv = (worldPos - uTerrainMaskOrigin) * uTerrainMaskInvSize;
    float cls = texture(uTerrainMaskTex, maskUv).r * 255.0;
    return int(floor(cls + 0.5));
}

void sampleMaskWeights(vec3 vtxColor, vec2 worldPos,
                       out float wWater, out float wUrban, out float wForest,
                       out float wFarmland, out float wRockClass) {
    wWater = 0.0;
    wUrban = 0.0;
    wForest = 0.0;
    wFarmland = 0.0;
    wRockClass = 0.0;
    if (!uTerrainUseMasks) {
        return;
    }
    if (uTerrainHasMaskTex) {
        float featherMeters = max(uTerrainMaskFeatherMeters, 1.0);
        float jitterMeters = max(uTerrainMaskJitterMeters, 0.0);
        vec2 jitter = vec2(noise(worldPos * 0.004),
                           noise(worldPos * 0.004 + vec2(12.3, 5.4)));
        jitter = (jitter - 0.5) * jitterMeters;
        vec2 offsets[13] = vec2[](
            vec2(0.0),
            vec2(featherMeters, 0.0),
            vec2(-featherMeters, 0.0),
            vec2(0.0, featherMeters),
            vec2(0.0, -featherMeters),
            vec2(featherMeters, featherMeters),
            vec2(-featherMeters, featherMeters),
            vec2(featherMeters, -featherMeters),
            vec2(-featherMeters, -featherMeters),
            vec2(featherMeters * 0.5, 0.0),
            vec2(-featherMeters * 0.5, 0.0),
            vec2(0.0, featherMeters * 0.5),
            vec2(0.0, -featherMeters * 0.5)
        );
        float samples = 0.0;
        for (int i = 0; i < 13; ++i) {
            int clsId = maskClassAt(worldPos + offsets[i] + jitter);
            wWater += clsId == 1 ? 1.0 : 0.0;
            wUrban += clsId == 2 ? 1.0 : 0.0;
            wForest += clsId == 3 ? 1.0 : 0.0;
            wFarmland += clsId == 5 ? 1.0 : 0.0;
            wRockClass += clsId == 6 ? 1.0 : 0.0;
            samples += 1.0;
        }
        wWater /= samples;
        wUrban /= samples;
        wForest /= samples;
        wFarmland /= samples;
        wRockClass /= samples;
    } else {
        wWater = clamp(vtxColor.r, 0.0, 1.0);
        wUrban = clamp(vtxColor.g, 0.0, 1.0);
        wForest = clamp(vtxColor.b, 0.0, 1.0);
    }
}

void main() {
    vec3 normal = normalize(vNormal);
    float roughMix = 0.5;
    float slope = 1.0 - dot(normal, vec3(0.0, 1.0, 0.0));
    vec3 lightDir = normalize(uLightDir);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 lighting = uAmbientColor + uLightColor * diffuse;
    vec3 baseColor = uUseUniformColor ? uColor : vColor;
    float wWater = 0.0;
    float wUrban = 0.0;
    float wForest = 0.0;
    float wFarmland = 0.0;
    float wRockClass = 0.0;
    sampleMaskWeights(vColor, vWorldPos.xz, wWater, wUrban, wForest, wFarmland, wRockClass);
    float wGrass = clamp(1.0 - (wWater + wUrban + wForest + (wFarmland * 0.75)), 0.0, 1.0);
    float edgeNoise = noise(vWorldPos.xz * 0.0009 + vec2(21.3, 4.7)) - 0.5;
    float farmlandSoft = smoothstep(0.15, 0.85, wFarmland + edgeNoise * uTerrainMaskEdgeNoise);
    wFarmland = mix(wFarmland, farmlandSoft, 0.6);
    if (uTerrainDebugMaskView) {
        vec3 debugColor = vec3(0.0);
        debugColor += wWater * vec3(0.1, 0.35, 0.8);
        debugColor += wUrban * vec3(0.6, 0.6, 0.6);
        debugColor += wForest * vec3(0.1, 0.5, 0.2);
        debugColor += wFarmland * vec3(0.72, 0.62, 0.28);
        debugColor += wRockClass * vec3(0.55, 0.5, 0.48);
        debugColor += wGrass * vec3(0.25, 0.65, 0.28);
        FragColor = vec4(debugColor, 1.0);
        return;
    }
    if (uTerrainShading && uTerrainUseTextures) {
        vec2 macroP = vWorldPos.xz * uTerrainMacroScale;
        float macro = noise(macroP);
        vec2 macroOffset = vec2(macro, noise(macroP + vec2(17.1, 9.7)));
        vec2 uv = vWorldPos.xz * uTerrainTexScale + macroOffset * 4.0;
        vec2 uvDetail = vWorldPos.xz * uTerrainDetailScale + macroOffset * 8.0;
        float rotPick = noise(macroP + vec2(31.7, 5.1)) * 4.0;
        float rotA = floor(rotPick);
        float rotB = mod(rotA + 1.0, 4.0);
        float rotBlend = smoothstep(0.25, 0.75, fract(rotPick));
        vec2 urbanUvA = rotate90(uv, rotA + 0.5);
        vec2 urbanUvB = rotate90(uv, rotB + 0.5);
        vec3 grassTex = texture(uTerrainTexGrass, uv).rgb;
        vec3 grassTexAlt = texture(uTerrainTexGrassB, uv * 1.35 + macroOffset * 3.5).rgb;
        vec3 forestTex = texture(uTerrainTexForest, uv).rgb;
        vec3 urbanTex = mix(texture(uTerrainTexUrban, urbanUvA).rgb,
                            texture(uTerrainTexUrban, urbanUvB).rgb,
                            rotBlend);
        vec3 rockTex = texture(uTerrainTexRock, uvDetail).rgb;
        vec3 dirtTex = texture(uTerrainTexDirt, uvDetail).rgb;
        vec3 gravelTex = texture(uTerrainTexRock, uvDetail * 1.7).rgb;
        float grassMix = smoothstep(0.2, 0.85, macro);
        vec3 grassTexB = mix(grassTex, grassTexAlt, grassMix);
        grassTexB = mix(grassTexB, gravelTex, 0.25);
        vec3 grassNormal = normalize(texture(uTerrainTexGrassNormal, uvDetail).xyz * 2.0 - 1.0);
        vec3 dirtNormal = normalize(texture(uTerrainTexDirtNormal, uvDetail).xyz * 2.0 - 1.0);
        vec3 rockNormal = normalize(texture(uTerrainTexRockNormal, uvDetail).xyz * 2.0 - 1.0);
        vec3 urbanNormal = normalize(texture(uTerrainTexUrbanNormal, uvDetail).xyz * 2.0 - 1.0);
        float grassR = texture(uTerrainTexGrassRough, uvDetail).r;
        float dirtR = texture(uTerrainTexDirtRough, uvDetail).r;
        float rockR = texture(uTerrainTexRockRough, uvDetail).r;
        float urbanR = texture(uTerrainTexUrbanRough, uvDetail).r;

        // Leave some grass even in farmland-heavy tiles to avoid flat monotony.
        float landSum = max(wGrass + wUrban + wForest + wFarmland, 0.0001);
        vec3 grassMixed = mix(grassTex, grassTexB, 0.2 + 0.25 * macro);
        vec3 farmlandBase = mix(grassMixed, dirtTex, 0.6);
        vec3 landColor = (grassMixed * wGrass + urbanTex * wUrban + forestTex * wForest + farmlandBase * wFarmland) / landSum;
        baseColor = mix(landColor, uTerrainWaterColor, wWater);

        // Elevation-driven tinting for broader biome feel (tempered by noise).
        float heightT = clamp((vWorldPos.y - uTerrainHeightMin) / max(uTerrainHeightMax - uTerrainHeightMin, 1.0), 0.0, 1.0);
        float lowBand = smoothstep(0.0, 0.22, heightT);
        float midBand = smoothstep(0.28, 0.6, heightT);
        float highBand = smoothstep(0.62, 0.9, heightT);
        vec3 lowTint = vec3(0.46, 0.44, 0.36);   // coastal soils
        vec3 dryTint = vec3(0.78, 0.70, 0.52);   // Bay Area golden hills
        vec3 highTint = vec3(0.76, 0.79, 0.82);  // desat highlands
        float biomeNoise = noise(vWorldPos.xz * 0.0006);
        baseColor = mix(baseColor, baseColor * lowTint, (1.0 - lowBand) * 0.22 * (0.6 + 0.4 * biomeNoise));
        baseColor = mix(baseColor, mix(baseColor, dryTint, 0.45), midBand * 0.35 * (0.7 + 0.3 * biomeNoise));
        baseColor = mix(baseColor, mix(baseColor, highTint, 0.35), highBand * 0.4);

        // Soft shoreline band for less razor-sharp water transitions.
        float shoreInner = smoothstep(0.08, uTerrainShoreWidth, wWater);
        float shoreOuter = 1.0 - smoothstep(uTerrainShoreWidth, uTerrainShoreWidth + uTerrainShoreFeather, wWater);
        float shore = shoreInner * shoreOuter;
        vec3 sandColor = mix(dirtTex, grassTexAlt, 0.25);
        baseColor = mix(baseColor, sandColor, shore * 0.65);
        baseColor = mix(baseColor, baseColor * 0.8, shore * uTerrainWetStrength);

        float macroGain = mix(1.0 - uTerrainMacroStrength, 1.0 + uTerrainMacroStrength, macro);
        float landWeight = clamp(wGrass + wUrban + wForest, 0.0, 1.0);
        baseColor = mix(baseColor, baseColor * macroGain, landWeight);

        float mega = noise(vWorldPos.xz * uTerrainMegaScale);
        float megaGain = mix(1.0 - uTerrainMegaStrength, 1.0 + uTerrainMegaStrength, mega);
        baseColor = mix(baseColor, baseColor * megaGain, landWeight);

        float tintMix = smoothstep(0.2, 0.8, macro);
        vec3 grassTint = mix(uTerrainGrassTintA, uTerrainGrassTintB, tintMix);
        vec3 forestTint = mix(uTerrainForestTintA, uTerrainForestTintB, tintMix);
        vec3 urbanTint = mix(uTerrainUrbanTintA, uTerrainUrbanTintB, tintMix);
        baseColor = mix(baseColor, baseColor * grassTint, uTerrainGrassTintStrength * wGrass);
        baseColor = mix(baseColor, baseColor * forestTint, uTerrainForestTintStrength * wForest);
        baseColor = mix(baseColor, baseColor * urbanTint, uTerrainUrbanTintStrength * wUrban);

        float micro = noise(vWorldPos.xz * uTerrainMicroScale);
        float microGain = mix(1.0 - uTerrainMicroStrength, 1.0 + uTerrainMicroStrength, micro);
        baseColor = mix(baseColor, baseColor * microGain, landWeight);

        float dirtNoise = noise(vWorldPos.xz * uTerrainDetailScale * 0.65);
        float dirtMask = smoothstep(0.35, 0.7, dirtNoise) * uTerrainDetailStrength;
        dirtMask *= clamp(wGrass + wForest, 0.0, 1.0);
        baseColor = mix(baseColor, dirtTex, dirtMask);

        float rockMask = smoothstep(uTerrainRockSlopeStart, uTerrainRockSlopeEnd, slope)
            * uTerrainRockStrength;
        rockMask = max(rockMask, wRockClass * 0.9);
        baseColor = mix(baseColor, rockTex, rockMask);

        vec3 blendNormal = vec3(0.0, 0.0, 1.0);
        roughMix = 0.0;
        float totalWeight = wGrass + wForest + wUrban + wFarmland + rockMask + 0.0001;
        blendNormal += grassNormal * (wGrass / totalWeight);
        blendNormal += dirtNormal * (wFarmland / totalWeight);
        blendNormal += urbanNormal * (wUrban / totalWeight);
        blendNormal += rockNormal * (rockMask / totalWeight);
        roughMix += grassR * (wGrass / totalWeight);
        roughMix += dirtR * (wFarmland / totalWeight);
        roughMix += urbanR * (wUrban / totalWeight);
        roughMix += rockR * (rockMask / totalWeight);
        blendNormal = normalize(blendNormal);
        normal = normalize(mat3(1.0) * blendNormal);

        float farmlandNoise = noise(vWorldPos.xz * uTerrainFarmlandStripeScale * 0.5);
        float farmlandPatch = noise(vWorldPos.xz * uTerrainMacroScale * 0.75);
        float tileSize = 1.0 / max(uTerrainMaskInvSize.x, 0.0001);
        vec2 tileId = floor((vWorldPos.xz - uTerrainMaskOrigin) * uTerrainMaskInvSize + vec2(0.0001));
        float tileHash = hash(tileId);
        float stripeAngle = tileHash * 6.2831853;
        vec2 stripeDir = normalize(vec2(cos(stripeAngle), sin(stripeAngle)) + vec2(0.2, 0.4));
        float stripe = sin(dot(vWorldPos.xz, stripeDir) * (uTerrainFarmlandStripeScale * 1100.0));
        float stripeMask = smoothstep(-uTerrainFarmlandStripeContrast, uTerrainFarmlandStripeContrast, stripe);
        float farmlandMask = wFarmland * uTerrainFarmlandStrength;
        float farmlandMix = clamp(farmlandMask * (0.45 + 0.2 * farmlandNoise) * (0.4 + 0.6 * farmlandPatch) * stripeMask, 0.0, 1.0);
        vec2 farmUv = vWorldPos.xz * uTerrainFarmTexScale + macroOffset * 2.0;
        vec3 farmTex = texture(uTerrainTexDirt, farmUv).rgb;
        vec3 farmlandColor = mix(grassTexB, farmTex, 0.6 + 0.1 * tileHash);
        baseColor = mix(baseColor, farmlandColor, farmlandMix);

        float scrubNoise = noise(vWorldPos.xz * uTerrainScrubNoiseScale + vec2(13.1, 7.2));
        float scrubMask = (1.0 - wWater) * (1.0 - wUrban) * (1.0 - wFarmland);
        scrubMask *= smoothstep(0.12, 0.5, slope) * scrubNoise * uTerrainScrubStrength * 1.2;
        baseColor = mix(baseColor, mix(baseColor, dirtTex, 0.5 + 0.2 * tileHash), clamp(scrubMask, 0.0, 1.0));

        float waterNoise = noise(vWorldPos.xz * uTerrainWaterDetailScale);
        float waterGain = mix(1.0 - uTerrainWaterDetailStrength, 1.0 + uTerrainWaterDetailStrength, waterNoise);
        baseColor = mix(baseColor, baseColor * waterGain, wWater);
    }
    if (uTerrainShading) {
        float luma = dot(baseColor, vec3(0.299, 0.587, 0.114));
        baseColor = mix(baseColor, vec3(luma), uTerrainDesaturate);
        baseColor = mix(baseColor, uTerrainTint, uTerrainTintStrength);

        float n = noise(vWorldPos.xz * uTerrainNoiseScale);
        baseColor *= (0.85 + uTerrainNoiseStrength * n);

        float heightRange = max(uTerrainHeightMax - uTerrainHeightMin, 1.0);
        float h = clamp((vWorldPos.y - uTerrainHeightMin) / heightRange, 0.0, 1.0);
        baseColor = mix(baseColor * 1.05, baseColor * 0.8, h);

        float rock = smoothstep(uTerrainSlopeStart, uTerrainSlopeEnd, slope);
        baseColor = mix(baseColor, baseColor * (1.0 - uTerrainSlopeDarken), rock);
    }

    vec3 litColor = baseColor * lighting;
    if (uTerrainShading) {
        // Apply simple roughness-driven contrast; roughMix already blended.
        litColor = mix(litColor, litColor * (0.9 + 0.2 * roughMix), 0.4);

        float d = distance(uCameraPos, vWorldPos);
        float distT = clamp((d - uTerrainDistanceDesatStart)
            / max(uTerrainDistanceDesatEnd - uTerrainDistanceDesatStart, 1.0), 0.0, 1.0);
        float distDesat = distT * uTerrainDistanceDesatStrength;
        float distContrast = 1.0 - distT * uTerrainDistanceContrastLoss;
        float litLuma = dot(litColor, vec3(0.299, 0.587, 0.114));
        litColor = mix(litColor, vec3(litLuma), distDesat);
        litColor = (litColor - 0.5) * distContrast + 0.5;

        float fog = clamp(d / uTerrainFogDistance, 0.0, 1.0);
        litColor = mix(litColor, uTerrainFogColor, fog);
    }
    FragColor = vec4(litColor, 1.0);
}
