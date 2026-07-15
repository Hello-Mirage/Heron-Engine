#version 450 core
layout(location = 0) in vec3 a_Position;

layout(binding = 1) uniform SceneData {
    mat4 u_ViewProjection;
    vec4 u_LightDir;
    vec4 u_LightColor;
    float u_LightIntensity;
    int u_HasLight;
};

layout(location = 0) out vec3 v_NearPoint;
layout(location = 1) out vec3 v_FarPoint;

vec3 UnprojectPoint(float x, float y, float z, mat4 invViewProj) {
    vec4 clipPoint = vec4(x, y, z, 1.0);
    vec4 worldPoint = invViewProj * clipPoint;
    return worldPoint.xyz / worldPoint.w;
}

void main() {
    mat4 invViewProj = inverse(u_ViewProjection);

    // Since we flipped the Vulkan viewport to match OpenGL (Y is up),
    // we don't need to manually flip the Y coordinate anymore!
    v_NearPoint = UnprojectPoint(a_Position.x, a_Position.y, -1.0, invViewProj);
    v_FarPoint  = UnprojectPoint(a_Position.x, a_Position.y, 1.0, invViewProj);
    
    gl_Position = vec4(a_Position, 1.0);
}
