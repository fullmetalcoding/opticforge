// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uNormalExaggeration; 

out vec3 vWorldPosition;
out vec3 vWorldNormal;

void main()
{
    vec4 worldPosition =
        uModel * vec4(aPosition, 1.0);

    vWorldPosition =
        worldPosition.xyz;

    // OpticForge optical transforms are currently rigid-body only.
    // If non-uniform scale is ever introduced, use:
    //
    // mat3 normalMatrix =
    //     transpose(inverse(mat3(uModel)));
    //
    // instead.
    
    //
    // Exaggerate curvature relative to local optical Z.
    //
    vec3 localNormal =
        normalize(
            vec3(
                aNormal.x * uNormalExaggeration,
                aNormal.y * uNormalExaggeration,
                aNormal.z));

    vWorldNormal =
        normalize(
            mat3(uModel) * localNormal);

    gl_Position =
        uProjection *
        uView *
        worldPosition;
}