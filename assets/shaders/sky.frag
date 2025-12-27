#version 330 core
in vec2 vUv;
out vec4 FragColor;

uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform vec3 uCameraForward;
uniform float uAspect;
uniform float uTanHalfFov;
uniform vec3 uSunDir;
uniform float uTime;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
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

float randomf(float x) {
    return fract(sin(x) * 43758.5453123);
}

float lightningPulse(vec3 dir, float time, float dayFactor) {
    vec3 boltDir = normalize(vec3(sin(time * 0.9), -0.3, cos(time * 1.1)));
    float alignment = max(dot(dir, boltDir), 0.0);
    float beam = pow(alignment, 28.0);

    float flicker = smoothstep(0.92, 1.0, randomf(time * 24.0));
    float trail = smoothstep(0.82, 0.97, dot(dir, vec3(0.0, -0.45, 1.0)));
    float intensity = (0.3 + 0.7 * randomf(time + 7.0)) * (1.0 - dayFactor);

    return beam * flicker * trail * intensity * 2.0;
}

void main() {
    vec2 ndc = vUv * 2.0 - 1.0;
    ndc.x *= uAspect;

    vec3 dir = normalize(uCameraForward
        + uCameraRight * ndc.x * uTanHalfFov
        + uCameraUp * ndc.y * uTanHalfFov);

    float elevation = clamp(uSunDir.y, -1.0, 1.0);
    float dayFactor = smoothstep(-0.25, 0.35, elevation);
    float duskFactor = smoothstep(-0.35, 0.12, elevation) * (1.0 - smoothstep(0.15, 0.5, elevation));

    vec3 zenithDay = vec3(0.10, 0.38, 0.78);
    vec3 horizonDay = vec3(0.72, 0.82, 0.94);
    vec3 zenithNight = vec3(0.01, 0.02, 0.05);
    vec3 horizonNight = vec3(0.04, 0.06, 0.12);
    vec3 zenithDusk = vec3(0.22, 0.12, 0.30);
    vec3 horizonDusk = vec3(0.96, 0.55, 0.32);

    vec3 zenith = mix(zenithNight, zenithDay, dayFactor);
    vec3 horizon = mix(horizonNight, horizonDay, dayFactor);
    zenith = mix(zenith, zenithDusk, duskFactor);
    horizon = mix(horizon, horizonDusk, duskFactor);

    float t = clamp((dir.y + 0.2) / 1.2, 0.0, 1.0);
    t = pow(t, 1.35);
    vec3 sky = mix(horizon, zenith, t);

    float haze = clamp((1.0 - dir.y) * 0.65, 0.0, 1.0);
    sky = mix(sky, horizon, haze * 0.25);

    float starNoise = noise(dir.xy * 40.0);
    float starMask = smoothstep(0.995, 1.0, starNoise) * (1.0 - dayFactor) * pow(1.0 - dir.y, 3.0);
    sky += vec3(0.75, 0.82, 1.0) * starMask;

    vec3 sunDir = normalize(uSunDir);
    float sunDot = max(dot(dir, sunDir), 0.0);
    float sunDisc = pow(sunDot, 2000.0);
    float sunGlow = pow(sunDot, 250.0);
    vec3 sunColor = mix(vec3(1.0, 0.95, 0.85), vec3(1.0, 0.65, 0.35), duskFactor);
    sky += sunColor * (sunGlow * 0.45 + sunDisc * 2.4);

    float lightning = lightningPulse(dir, uTime, dayFactor);
    vec3 lightningColor = vec3(1.0, 0.98, 0.85);
    sky += lightningColor * lightning;

    sky = clamp(sky, 0.0, 1.0);
    FragColor = vec4(sky, 1.0);
}
