#version 450 core
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

layout(location = 0) out vec4 FragColor;
void main() { 
    FragColor = u_Color;
}
