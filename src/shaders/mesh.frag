#version 330 core

in vec3 vWorldPosition;
in vec3 vWorldNormal;

out vec4 FragColor;

uniform vec3 uCameraPosition;

// Material
uniform vec3 uBaseColor;
uniform float uOpacity;

uniform float uAmbientStrength;
uniform float uDiffuseStrength;
uniform float uSpecularStrength;
uniform float uShininess;

// Simple directional light.
uniform vec3 uLightDirection;
uniform vec3 uLightColor;

// Optional edge brightening.
// Useful especially for transparent lenses.
uniform float uFresnelStrength;
uniform float uFresnelPower;

void main()
{
    vec3 normal =
        normalize(vWorldNormal);

    vec3 viewDirection =
        normalize(
            uCameraPosition -
            vWorldPosition);

    // uLightDirection represents the direction FROM the surface
    // TOWARD the light.
    vec3 lightDirection =
        normalize(uLightDirection);

    // Ambient
    vec3 ambient =
        uAmbientStrength *
        uBaseColor;

    // Diffuse
    float diffuseFactor =
        max(
            dot(
                normal,
                lightDirection),
            0.0);

    vec3 diffuse =
        uDiffuseStrength *
        diffuseFactor *
        uBaseColor *
        uLightColor;

    // Blinn-Phong specular.
    vec3 halfwayDirection =
        normalize(
            lightDirection +
            viewDirection);

    float specularFactor =
        pow(
            max(
                dot(
                    normal,
                    halfwayDirection),
                0.0),
            uShininess);

    vec3 specular =
        uSpecularStrength *
        specularFactor *
        uLightColor;

    // Fresnel-style edge enhancement.
    float facing =
        abs(
            dot(
                normal,
                viewDirection));

    float fresnel =
        pow(
            1.0 - facing,
            uFresnelPower);

    vec3 fresnelColor =
        uBaseColor *
        fresnel *
        uFresnelStrength;

    vec3 finalColor =
        ambient +
        diffuse +
        specular +
        fresnelColor;

    FragColor =
        vec4(
            finalColor,
            uOpacity);
}