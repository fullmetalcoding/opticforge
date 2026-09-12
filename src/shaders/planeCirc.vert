#version 330 core

uniform mat4 uView;
uniform mat4 uProjection;

uniform vec3 uCenter;
uniform vec3 uNormal;

uniform float uRadius;

out vec2 vUV;
out vec3 vWorldPosition;
out vec3 vWorldNormal;

void main()
{
    // Triangle-strip corners:
    //
    // 2 ---- 3
    // |      |
    // |      |
    // 0 ---- 1
    //
    const vec2 corners[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
    );

    vec2 corner = corners[gl_VertexID];

    vec3 N = normalize(uNormal);

    /*
     * Build an orthonormal basis lying in the plane.
     *
     * We choose a helper vector that isn't nearly parallel
     * to the supplied normal.
     */
    vec3 helper =
        (abs(N.y) < 0.999)
        ? vec3(0.0, 1.0, 0.0)
        : vec3(1.0, 0.0, 0.0);

    vec3 tangent =
        normalize(cross(helper, N));

    vec3 bitangent =
        normalize(cross(N, tangent));

    /*
     * 'corner' spans [-1,+1], so multiplying by radius
     * produces a square of side 2 * radius.
     *
     * The fragment shader clips this square into a circle.
     */
    vec3 worldPosition =
        uCenter +
        tangent   * (corner.x * uRadius) +
        bitangent * (corner.y * uRadius);

    vWorldPosition = worldPosition;
    vWorldNormal = N;

    /*
     * UV coordinates spanning [0,1].
     */
    vUV =
        corner * 0.5 + 0.5;

    gl_Position =
        uProjection *
        uView *
        vec4(worldPosition, 1.0);
}