#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

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

layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec3 v_WorldPos;
layout(location = 2) out vec2 v_TexCoord;

void main() {
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
    v_TexCoord = a_TexCoord;
    gl_Position = u_ViewProjection * worldPos;
}
