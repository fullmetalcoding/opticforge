// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#version 330 core

out vec2 vNdc;

void main()
{
    const vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );

    vec2 p = positions[gl_VertexID];

    vNdc = p;

    gl_Position = vec4(
        p,
        0.0,
        1.0);
}