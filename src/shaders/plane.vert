#version 330 core

uniform mat4 uView;
uniform mat4 uProjection;

uniform vec3 uCenter;
uniform vec3 uNormal;
uniform vec2 uSize;   // width, height

out vec3 vWorldPos;
out vec3 vWorldNormal;
out vec2 vUV;

void main()
{
    // Quad corners for a triangle strip:
    // 0 = (-1,-1)
    // 1 = ( 1,-1)
    // 2 = (-1, 1)
    // 3 = ( 1, 1)
    const vec2 corners[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
    );

    vec2 corner = corners[gl_VertexID];

    vec3 N = normalize(uNormal);

    // Build an orthonormal basis from the normal.
    // Pick a helper axis that is not parallel to N.
    vec3 helper =
        (abs(N.y) < 0.999)
        ? vec3(0.0, 1.0, 0.0)
        : vec3(1.0, 0.0, 0.0);

    vec3 T = normalize(cross(helper, N));
    vec3 B = normalize(cross(N, T));

    vec2 halfSize = 0.5 * uSize;

    vec3 worldPos =
        uCenter +
        T * (corner.x * halfSize.x) +
        B * (corner.y * halfSize.y);

    vWorldPos = worldPos;
    vWorldNormal = N;
    vUV = corner * 0.5 + 0.5;

    gl_Position =
        uProjection *
        uView *
        vec4(worldPos, 1.0);
}