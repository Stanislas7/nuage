#version 330 core
in vec2 vUv;
out vec4 FragColor;

uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform vec3 uCameraForward;
uniform float uAspect;
uniform float uTanHalfFov;
uniform vec3 uSunDir;

void main() {
    vec2 ndc = vUv * 2.0 - 1.0;
    ndc.x *= uAspect;

    vec3 dir = normalize(uCameraForward
        + uCameraRight * ndc.x * uTanHalfFov
        + uCameraUp * ndc.y * uTanHalfFov);

    float sunElev = clamp(uSunDir.y, -1.0, 1.0);
    float dayFactor = smoothstep(-0.1, 0.2, sunElev);
    float duskFactor = smoothstep(-0.2, 0.1, sunElev) * (1.0 - smoothstep(0.1, 0.4, sunElev));

    vec3 zenithDay = vec3(0.10, 0.35, 0.70);
    vec3 horizonDay = vec3(0.72, 0.82, 0.92);
    vec3 zenithNight = vec3(0.01, 0.02, 0.05);
    vec3 horizonNight = vec3(0.05, 0.06, 0.10);
    vec3 zenithDusk = vec3(0.18, 0.10, 0.25);
    vec3 horizonDusk = vec3(0.95, 0.55, 0.30);

    vec3 zenith = mix(zenithNight, zenithDay, dayFactor);
    vec3 horizon = mix(horizonNight, horizonDay, dayFactor);
    zenith = mix(zenith, zenithDusk, duskFactor * 0.5);
    horizon = mix(horizon, horizonDusk, duskFactor);

    float t = clamp((dir.y + 0.05) / 1.05, 0.0, 1.0);
    t = pow(t, 1.4);

    vec3 sky = mix(horizon, zenith, t);

    vec3 sunDir = normalize(uSunDir);
    float sunDot = max(dot(dir, sunDir), 0.0);
    float sunDisc = pow(sunDot, 2000.0);
    float sunGlow = pow(sunDot, 200.0);
    vec3 sunColor = mix(vec3(1.0, 0.95, 0.85), vec3(1.0, 0.65, 0.35), duskFactor);
    sky += sunColor * (sunGlow * 0.35 + sunDisc * 2.0);

    float haze = clamp(1.0 - dir.y, 0.0, 1.0);
    sky = mix(sky, horizon, haze * 0.2);

    FragColor = vec4(sky, 1.0);
}
