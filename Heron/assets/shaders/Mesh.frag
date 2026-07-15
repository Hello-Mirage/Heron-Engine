#version 450 core
layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec3 v_WorldPos;
layout(location = 2) in vec2 v_TexCoord;

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

layout(binding = 0) uniform sampler2D u_Texture;

layout(location = 0) out vec4 FragColor;

void main() {
    vec3 normal = normalize(v_Normal);
    vec3 lighting = vec3(1.0); // Default to full bright if no lights

    if (u_HasLight != 0) {
        // Ambient
        float ambientStrength = 0.2;
        vec3 ambient = ambientStrength * u_LightColor.xyz;

        // Diffuse (Directional Light)
        vec3 lightDir = normalize(-u_LightDir.xyz); // Direction TO the light
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * u_LightColor.xyz;
        
        lighting = (ambient + diffuse) * u_LightIntensity;
    } else {
        // Fallback directional lighting if no light exists
        vec3 lightDir = normalize(vec3(0.4, 0.8, 0.3));
        float diff = max(dot(normal, lightDir), 0.0);
        float ambient = 0.25;
        lighting = vec3(ambient + diff * 0.75);
    }

    vec4 texColor = texture(u_Texture, v_TexCoord);
    FragColor = texColor * vec4(u_Color.rgb * lighting, u_Color.a);
}
