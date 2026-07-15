#version 450 core
layout(location = 0) in vec3 v_NearPoint;
layout(location = 1) in vec3 v_FarPoint;

layout(binding = 1) uniform SceneData {
    mat4 u_ViewProjection;
    vec4 u_LightDir;
    vec4 u_LightColor;
    float u_LightIntensity;
    int u_HasLight;
};

layout(location = 0) out vec4 FragColor;

float ComputeDepth(vec3 pos) {
    vec4 clipPos = u_ViewProjection * vec4(pos, 1.0);
    return clipPos.z / clipPos.w;
}

float ComputeLinearDepth(vec3 pos) {
    vec4 clipPos = u_ViewProjection * vec4(pos, 1.0);
    // Vulkan uses [0, 1] depth now, so we map it back to [-1, 1] for the standard OpenGL linearization formula!
    float clipDepth = (clipPos.z / clipPos.w) * 2.0 - 1.0;
    float near = 0.1;
    float far = 1000.0;
    float linearDepth = (2.0 * near * far) / (far + near - clipDepth * (far - near));
    return linearDepth / far;
}

vec4 Grid(vec3 fragPos3D, float scale) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumx = min(derivative.x, 1.0);
    float minimumz = min(derivative.y, 1.0);
    vec4 color = vec4(0.3, 0.3, 0.3, 1.0 - min(line, 1.0));
    if (fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        color.xyz = vec3(0.8, 0.2, 0.2);
    if (fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        color.xyz = vec3(0.2, 0.2, 0.8);
    return color;
}

void main() {
    float t = -v_NearPoint.y / (v_FarPoint.y - v_NearPoint.y);
    if (t < 0.0) discard;
    vec3 fragPos3D = v_NearPoint + t * (v_FarPoint - v_NearPoint);
    gl_FragDepth = ComputeDepth(fragPos3D);
    float linearDepth = ComputeLinearDepth(fragPos3D);
    float fade = max(0.0, 0.5 - linearDepth);
    FragColor = Grid(fragPos3D, 1.0) + Grid(fragPos3D, 0.1);
    FragColor.a *= fade;
}
