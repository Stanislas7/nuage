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
uniform bool uTerrainHasMaskTex = false;
uniform sampler2D uTerrainMaskTex;
uniform vec2 uTerrainMaskOrigin;
uniform vec2 uTerrainMaskInvSize;

uniform sampler2DArray uTerrainTexArray;
uniform int uLandclassTexCount[256];
uniform int uLandclassTexIndex0[256];
uniform int uLandclassTexIndex1[256];
uniform int uLandclassTexIndex2[256];
uniform int uLandclassTexIndex3[256];
uniform float uLandclassTexScale[256];

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

int sampleLandclass(vec2 worldPos) {
    if (!uTerrainUseMasks || !uTerrainHasMaskTex) {
        return 0;
    }
    vec2 maskUv = (worldPos - uTerrainMaskOrigin) * uTerrainMaskInvSize;
    float cls = texture(uTerrainMaskTex, maskUv).r * 255.0;
    return int(floor(cls + 0.5));
}

int selectTextureIndex(int landclass, float picker) {
    int count = uLandclassTexCount[landclass];
    if (count <= 1) {
        return uLandclassTexIndex0[landclass];
    }
    int slot = int(floor(picker * float(count)));
    slot = clamp(slot, 0, count - 1);
    if (slot == 1) return uLandclassTexIndex1[landclass];
    if (slot == 2) return uLandclassTexIndex2[landclass];
    if (slot == 3) return uLandclassTexIndex3[landclass];
    return uLandclassTexIndex0[landclass];
}

void main() {
    vec3 normal = normalize(vNormal);
    float slope = 1.0 - dot(normal, vec3(0.0, 1.0, 0.0));
    vec3 lightDir = normalize(uLightDir);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 lighting = uAmbientColor + uLightColor * diffuse;
    vec3 baseColor = uUseUniformColor ? uColor : vColor;

    if (uTerrainUseTextures) {
        int landclass = sampleLandclass(vWorldPos.xz);
        float texScale = uLandclassTexScale[landclass];
        if (texScale <= 0.0) {
            texScale = 0.0005;
        }
        float picker = hash(floor(vWorldPos.xz * 0.002));
        int layer = selectTextureIndex(landclass, picker);
        vec2 uv = vWorldPos.xz * texScale;
        baseColor = texture(uTerrainTexArray, vec3(uv, float(layer))).rgb;
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
