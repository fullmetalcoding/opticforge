// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#version 330 core

uniform vec3 uColor;
uniform float uOpacity;

in vec2 vUV;
in vec3 vWorldPosition;
in vec3 vWorldNormal;

out vec4 FragColor;

void main()
{
    /*
     * Convert [0,1] UV coordinates to [-1,+1].
     *
     * The circle center is therefore (0,0) and
     * its radius is 1 in local UV space.
     */
    vec2 p =
        vUV * 2.0 - 1.0;

    float radius =
        length(p);

    /*
     * Use screen-space derivatives to get an approximately
     * one-pixel-wide antialiased edge.
     */
    float edgeWidth =
        fwidth(radius);

    float coverage =
        1.0 -
        smoothstep(
            1.0 - edgeWidth,
            1.0 + edgeWidth,
            radius);

    if (coverage <= 0.001)
        discard;

    FragColor =
        vec4(
            uColor,
            uOpacity * coverage);
}