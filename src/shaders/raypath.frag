// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#version 330 core
flat in int vCategory;
uniform vec4 uObservationColor;
uniform vec4 uEscapedColor;
uniform vec4 uTerminatedColor;
out vec4 FragColor;
void main()
{
    FragColor = vCategory == 0 ? uObservationColor
        : (vCategory == 1 ? uEscapedColor : uTerminatedColor);
}