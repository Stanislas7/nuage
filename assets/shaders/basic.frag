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
uniform sampler2D uTerrainTexForest;
uniform sampler2D uTerrainTexRock;
uniform sampler2D uTerrainTexDirt;
uniform sampler2D uTerrainTexUrban;
uniform float uTerrainTexScale = 0.02;
uniform float uTerrainDetailScale = 0.08;
uniform float uTerrainDetailStrength = 0.35;
uniform float uTerrainRockSlopeStart = 0.35;
uniform float uTerrainRockSlopeEnd = 0.7;
uniform float uTerrainRockStrength = 0.7;
uniform vec3 uTerrainWaterColor = vec3(0.14, 0.32, 0.55);

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

void main() {
    vec3 normal = normalize(vNormal);
    float slope = 1.0 - dot(normal, vec3(0.0, 1.0, 0.0));
    vec3 lightDir = normalize(uLightDir);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 lighting = uAmbientColor + uLightColor * diffuse;
    vec3 baseColor = uUseUniformColor ? uColor : vColor;
    if (uTerrainShading && uTerrainUseTextures) {
        vec2 uv = vWorldPos.xz * uTerrainTexScale;
        vec2 uvDetail = vWorldPos.xz * uTerrainDetailScale;
        vec3 grassTex = texture(uTerrainTexGrass, uv).rgb;
        vec3 forestTex = texture(uTerrainTexForest, uv).rgb;
        vec3 urbanTex = texture(uTerrainTexUrban, uv).rgb;
        vec3 rockTex = texture(uTerrainTexRock, uvDetail).rgb;
        vec3 dirtTex = texture(uTerrainTexDirt, uvDetail).rgb;

        float wWater = 0.0;
        float wUrban = 0.0;
        float wForest = 0.0;
        if (uTerrainUseMasks) {
            wWater = clamp(vColor.r, 0.0, 1.0);
            wUrban = clamp(vColor.g, 0.0, 1.0);
            wForest = clamp(vColor.b, 0.0, 1.0);
        }
        float wGrass = clamp(1.0 - (wWater + wUrban + wForest), 0.0, 1.0);
        float landSum = max(wGrass + wUrban + wForest, 0.0001);
        vec3 landColor = (grassTex * wGrass + urbanTex * wUrban + forestTex * wForest) / landSum;
        baseColor = mix(landColor, uTerrainWaterColor, wWater);

        float dirtNoise = noise(vWorldPos.xz * uTerrainDetailScale * 0.65);
        float dirtMask = smoothstep(0.35, 0.7, dirtNoise) * uTerrainDetailStrength;
        dirtMask *= clamp(wGrass + wForest, 0.0, 1.0);
        baseColor = mix(baseColor, dirtTex, dirtMask);

        float rockMask = smoothstep(uTerrainRockSlopeStart, uTerrainRockSlopeEnd, slope)
            * uTerrainRockStrength;
        baseColor = mix(baseColor, rockTex, rockMask);
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
