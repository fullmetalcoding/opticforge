// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in float aCategory;
uniform mat4 uView;
uniform mat4 uProjection;
flat out int vCategory;
void main()
{
    gl_Position = uProjection * uView * vec4(aPosition, 1.0);
    vCategory = int(aCategory + 0.5);
}