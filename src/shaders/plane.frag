// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#version 330 core

uniform vec3 uColor;
uniform float uOpacity;

in vec3 vWorldPos;
in vec3 vWorldNormal;
in vec2 vUV;

out vec4 FragColor;

void main()
{
    vec4 base = vec4(uColor, uOpacity);

    float edgeDist = min(
        min(vUV.x, 1.0 - vUV.x),
        min(vUV.y, 1.0 - vUV.y));

    float borderWidth = 0.03;
    float border = 1.0 - smoothstep(0.0, borderWidth, edgeDist);

    vec3 borderColor = min(uColor * 1.35, vec3(1.0));
    vec3 finalColor = mix(base.rgb, borderColor, border);

    FragColor = vec4(finalColor, base.a);
}