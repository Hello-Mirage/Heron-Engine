#version 450 core
layout(location = 0) in vec3 a_Position;
layout(binding = 1) uniform SceneData {
    mat4 u_ViewProjection;
    vec4 u_LightDir;
    vec4 u_LightColor;
    float u_LightIntensity;
    int u_HasLight;
};

layout(push_constant) uniform constants {
    mat4 u_Model;
    vec4 u_Color;
};
void main() { gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0); }
