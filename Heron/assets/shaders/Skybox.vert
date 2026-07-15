#version 450 core
layout (location = 0) in vec3 a_Position;

layout(location = 0) out vec3 v_TexCoords;

layout(binding = 1) uniform SceneData {
    mat4 u_ViewProjection;
    vec4 u_LightDir;
    vec4 u_LightColor;
    float u_LightIntensity;
    int u_HasLight;
};

void main() {
    v_TexCoords = a_Position;
    vec4 pos = u_ViewProjection * vec4(a_Position, 1.0);
    gl_Position = pos.xyww; // Force z to w, so it renders at depth = 1.0
}
