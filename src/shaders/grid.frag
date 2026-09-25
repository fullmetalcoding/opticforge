// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#version 330 core

in vec2 vNdc;

out vec4 FragColor;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uInvView;
uniform mat4 uInvProjection;

uniform float uMinorSpacing;
uniform float uMajorSpacing;

uniform vec3 uMinorColor;
uniform vec3 uMajorColor;

uniform float uFadeDistance;
uniform float uOpacity;

vec3 worldPositionFromNdc(
    vec2 ndc,
    float ndcZ)
{
    vec4 viewPos =
        uInvProjection *
        vec4(
            ndc,
            ndcZ,
            1.0);

    viewPos /= viewPos.w;

    vec4 worldPos =
        uInvView *
        viewPos;

    return worldPos.xyz;
}

float gridLine(
    vec2 coord,
    float spacing)
{
    vec2 scaled =
        coord / spacing;

    vec2 distanceToLine =
        abs(
            fract(scaled - 0.5)
            - 0.5);

    vec2 derivative =
        fwidth(scaled);

    vec2 line =
        1.0 -
        smoothstep(
            vec2(0.0),
            derivative,
            distanceToLine);

    return max(
        line.x,
        line.y);
}

void main()
{
    vec3 nearPoint =
        worldPositionFromNdc(
            vNdc,
            -1.0);

    vec3 farPoint =
        worldPositionFromNdc(
            vNdc,
            1.0);

    vec3 rayDirection =
        normalize(
            farPoint -
            nearPoint);

    // Nearly parallel with Y=0 plane.
    if (abs(rayDirection.y) < 1e-6)
        discard;

    float t =
        -nearPoint.y /
        rayDirection.y;

    // Ground plane lies behind camera.
    if (t <= 0.0)
        discard;

    vec3 worldPosition =
        nearPoint +
        t * rayDirection;

        
    // ---------------------------------------------
    // Give the procedural grid its actual 3D depth
    // ---------------------------------------------

    vec4 clip =
        uProjection *
        uView *
        vec4(worldPosition, 1.0);

    float ndcDepth =
        clip.z / clip.w;

    gl_FragDepth =
        ndcDepth * 0.5 + 0.5;

    vec2 groundCoordinate =
        worldPosition.xz;

    float minor =
        gridLine(
            groundCoordinate,
            uMinorSpacing);

    float major =
        gridLine(
            groundCoordinate,
            uMajorSpacing);

    float grid =
        max(
            minor,
            major);

    vec3 color =
        mix(
            uMinorColor,
            uMajorColor,
            major);

    vec3 cameraPosition =
        (
            uInvView *
            vec4(
                0.0,
                0.0,
                0.0,
                1.0)
        ).xyz;

    float cameraDistance =
        length(
            worldPosition -
            cameraPosition);

    float fade =
        1.0 -
        smoothstep(
            uFadeDistance * 0.25,
            uFadeDistance,
            cameraDistance);

    float alpha =
        grid *
        fade *
        uOpacity;

    if (alpha < 0.001)
        discard;

    FragColor =
        vec4(
            color,
            alpha);
}