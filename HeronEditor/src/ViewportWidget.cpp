#include <glad/gl.h>
#include "ViewportWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QLabel>

#include <windows.h>
#include <vulkan/vulkan_win32.h>
#include <Heron/Platform/Vulkan/VulkanContext.h>
#include <Heron/Platform/Vulkan/VulkanShader.h>
#include <Heron/Core/Log.h>
#include <Heron/Renderer/EditorCamera.h>
#include <Heron/Renderer/Texture.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

QLabel* g_ScoreLabel = nullptr;

ViewportWidget::ViewportWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_NoSystemBackground);

    m_ActiveScene = Heron::CreateRef<Heron::Scene>();

    m_ScoreLabel = new QLabel("Score: 0", this);
    m_ScoreLabel->setStyleSheet("QLabel { color: white; font-size: 24px; font-weight: bold; background-color: rgba(0, 0, 0, 150); padding: 10px; border-radius: 5px; }");
    m_ScoreLabel->move(20, 20);
    m_ScoreLabel->hide();
    g_ScoreLabel = m_ScoreLabel;

    connect(&m_Timer, &QTimer::timeout, this, &ViewportWidget::updateGameLoop);
    m_Timer.start(16);
}

ViewportWidget::~ViewportWidget()
{
}

void ViewportWidget::SetScene(Heron::Ref<Heron::Scene> scene)
{
    emit SceneChanged(scene.get());
    m_ActiveScene = scene;
    m_EditorScene = scene;
}

static void* __stdcall QtGLGetProcAddress(const char* name) {
    if (QOpenGLContext::currentContext()) {
        return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
    }
    return nullptr;
}

// ============================================================
//  MESH SHADER — simple solid color with MVP transform
// ============================================================
void ViewportWidget::CreateMeshShader()
{
    std::string vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        layout(location = 2) in vec2 a_TexCoord;

        uniform mat4 u_ViewProjection;
        uniform mat4 u_Model;

        out vec3 v_Normal;
        out vec3 v_WorldPos;
        out vec2 v_TexCoord;

        void main() {
            vec4 worldPos = u_Model * vec4(a_Position, 1.0);
            v_WorldPos = worldPos.xyz;
            v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
            v_TexCoord = a_TexCoord;
            gl_Position = u_ViewProjection * worldPos;
        }
    )";

    std::string fragSrc = R"(
        #version 330 core
        in vec3 v_Normal;
        in vec3 v_WorldPos;
        in vec2 v_TexCoord;

        uniform vec4 u_Color;
        uniform sampler2D u_Texture;
        
        uniform vec3 u_LightPos;
        uniform vec3 u_LightColor;
        uniform float u_LightIntensity;
        uniform bool u_HasLight;

        layout(location = 0) out vec4 FragColor;

        void main() {
            vec3 normal = normalize(v_Normal);
            vec3 lighting = vec3(1.0); // Default to full bright if no lights

            if (u_HasLight) {
                // Ambient
                float ambientStrength = 0.1;
                vec3 ambient = ambientStrength * u_LightColor;

                // Diffuse
                vec3 lightDir = normalize(u_LightPos - v_WorldPos);
                float diff = max(dot(normal, lightDir), 0.0);
                vec3 diffuse = diff * u_LightColor;
                
                // Attenuation
                float distance = length(u_LightPos - v_WorldPos);
                float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));
                
                lighting = (ambient + diffuse) * attenuation * u_LightIntensity;
            } else {
                // Fallback directional lighting if no point light exists
                vec3 lightDir = normalize(vec3(0.4, 0.8, 0.3));
                float diff = max(dot(normal, lightDir), 0.0);
                float ambient = 0.25;
                lighting = vec3(ambient + diff * 0.75);
            }

            vec4 texColor = texture(u_Texture, v_TexCoord);
            FragColor = texColor * vec4(u_Color.rgb * lighting, u_Color.a);
        }
    )";

	if (Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan) {
		m_MeshShader = Heron::Shader::Create("assets/shaders/Mesh.vert.spv", "assets/shaders/Mesh.frag.spv");
	} else {
		m_MeshShader = Heron::Shader::Create(vertSrc, fragSrc);
	}
}

void ViewportWidget::CreateSkyboxShader()
{
    std::string vertSrc = R"(
        #version 410 core
        layout (location = 0) in vec3 a_Position;

        out vec3 v_TexCoords;

        uniform mat4 u_Projection;
        uniform mat4 u_View;

        void main() {
            v_TexCoords = a_Position;
            vec4 pos = u_Projection * u_View * vec4(a_Position, 1.0);
            gl_Position = pos.xyww; // Force z to w, so it renders at depth = 1.0
        }
    )";

    std::string fragSrc = R"(
        #version 410 core
        out vec4 FragColor;

        in vec3 v_TexCoords;
        uniform samplerCube u_Skybox;

        void main() {
            FragColor = texture(u_Skybox, v_TexCoords);
        }
    )";

	if (Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan) {
		m_SkyboxShader = Heron::Shader::Create("assets/shaders/Skybox.vert.spv", "assets/shaders/Skybox.frag.spv");
	} else {
		m_SkyboxShader = Heron::Shader::Create(vertSrc, fragSrc);
	}
}

// ============================================================
//  PRIMITIVE MESH GENERATION — Cube, Plane, Cylinder
// ============================================================
void ViewportWidget::GeneratePrimitiveMeshes()
{
    // ---- CUBE (24 vertices with normals & UVs for flat shading) ----
    {
        float s = 0.5f;
        // pos(3) + normal(3) + uv(2) = 8 floats per vertex, 6 faces * 4 verts = 24 verts
        float cubeVerts[] = {
            // Front face (+Z)
            -s,-s, s,  0, 0, 1, 0,0,   s,-s, s,  0, 0, 1, 1,0,   s, s, s,  0, 0, 1, 1,1,  -s, s, s,  0, 0, 1, 0,1,
            // Back face (-Z)
             s,-s,-s,  0, 0,-1, 0,0,  -s,-s,-s,  0, 0,-1, 1,0,  -s, s,-s,  0, 0,-1, 1,1,   s, s,-s,  0, 0,-1, 0,1,
            // Top face (+Y)
            -s, s, s,  0, 1, 0, 0,0,   s, s, s,  0, 1, 0, 1,0,   s, s,-s,  0, 1, 0, 1,1,  -s, s,-s,  0, 1, 0, 0,1,
            // Bottom face (-Y)
            -s,-s,-s,  0,-1, 0, 0,0,   s,-s,-s,  0,-1, 0, 1,0,   s,-s, s,  0,-1, 0, 1,1,  -s,-s, s,  0,-1, 0, 0,1,
            // Right face (+X)
             s,-s, s,  1, 0, 0, 0,0,   s,-s,-s,  1, 0, 0, 1,0,   s, s,-s,  1, 0, 0, 1,1,   s, s, s,  1, 0, 0, 0,1,
            // Left face (-X)
            -s,-s,-s, -1, 0, 0, 0,0,  -s,-s, s, -1, 0, 0, 1,0,  -s, s, s, -1, 0, 0, 1,1,  -s, s,-s, -1, 0, 0, 0,1,
        };
        uint32_t cubeIdx[] = {
             0, 1, 2, 2, 3, 0,   // Front
             4, 5, 6, 6, 7, 4,   // Back
             8, 9,10,10,11, 8,   // Top
            12,13,14,14,15,12,   // Bottom
            16,17,18,18,19,16,   // Right
            20,21,22,22,23,20,   // Left
        };

        auto vao = Heron::VertexArray::Create();
        auto vbo = Heron::VertexBuffer::Create(cubeVerts, sizeof(cubeVerts));
        vbo->SetLayout({
            { Heron::ShaderDataType::Float3, "a_Position" },
            { Heron::ShaderDataType::Float3, "a_Normal" },
            { Heron::ShaderDataType::Float2, "a_TexCoord" }
        });
        vao->AddVertexBuffer(vbo);
        auto ibo = Heron::IndexBuffer::Create(cubeIdx, 36);
        vao->SetIndexBuffer(ibo);

        m_PrimitiveMeshes[(int)Heron::PrimitiveType::Cube] = vao;
    }

    // ---- PLANE (4 vertices, slightly above 0 to avoid being hidden by the grid) ----
    {
        float planeVerts[] = {
            // pos + normal + uv
            -5.0f, 0.001f, -5.0f,  0, 1, 0,  0.0f, 0.0f,
             5.0f, 0.001f, -5.0f,  0, 1, 0,  1.0f, 0.0f,
             5.0f, 0.001f,  5.0f,  0, 1, 0,  1.0f, 1.0f,
            -5.0f, 0.001f,  5.0f,  0, 1, 0,  0.0f, 1.0f,
        };
        uint32_t planeIdx[] = { 0, 1, 2, 2, 3, 0 };

        auto vao = Heron::VertexArray::Create();
        auto vbo = Heron::VertexBuffer::Create(planeVerts, sizeof(planeVerts));
        vbo->SetLayout({
            { Heron::ShaderDataType::Float3, "a_Position" },
            { Heron::ShaderDataType::Float3, "a_Normal" },
            { Heron::ShaderDataType::Float2, "a_TexCoord" }
        });
        vao->AddVertexBuffer(vbo);
        auto ibo = Heron::IndexBuffer::Create(planeIdx, 6);
        vao->SetIndexBuffer(ibo);

        m_PrimitiveMeshes[(int)Heron::PrimitiveType::Plane] = vao;
    }

    // ---- CYLINDER (generated procedurally) ----
    {
        const int segments = 24;
        const float radius = 0.5f;
        const float height = 1.0f;
        const float halfH = height / 2.0f;

        // We need: top ring, bottom ring, top cap center, bottom cap center
        // Side faces: segments * 2 verts (top + bottom rings)
        // Cap faces: segments verts + 1 center each
        std::vector<float> verts;
        std::vector<uint32_t> indices;

        // --- Side vertices ---
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * (float)M_PI;
            float nx = cosf(angle);
            float nz = sinf(angle);
            float x = radius * nx;
            float z = radius * nz;
            float u = (float)i / segments;

            // Top vertex
            verts.insert(verts.end(), { x, halfH, z, nx, 0, nz, u, 1.0f });
            // Bottom vertex
            verts.insert(verts.end(), { x, -halfH, z, nx, 0, nz, u, 0.0f });
        }

        // Side indices
        for (int i = 0; i < segments; i++) {
            uint32_t top1 = i * 2;
            uint32_t bot1 = i * 2 + 1;
            uint32_t top2 = (i + 1) * 2;
            uint32_t bot2 = (i + 1) * 2 + 1;
            indices.insert(indices.end(), { top1, bot1, top2, top2, bot1, bot2 });
        }

        // --- Top cap ---
        uint32_t topCenter = (uint32_t)(verts.size() / 8);
        verts.insert(verts.end(), { 0, halfH, 0, 0, 1, 0, 0.5f, 0.5f }); // center
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * (float)M_PI;
            float x = radius * cosf(angle);
            float z = radius * sinf(angle);
            float u = 0.5f + 0.5f * cosf(angle);
            float v = 0.5f + 0.5f * sinf(angle);
            verts.insert(verts.end(), { x, halfH, z, 0, 1, 0, u, v });
        }
        for (int i = 0; i < segments; i++) {
            indices.insert(indices.end(), { topCenter, topCenter + 1 + (uint32_t)i, topCenter + 2 + (uint32_t)i });
        }

        // --- Bottom cap ---
        uint32_t botCenter = (uint32_t)(verts.size() / 8);
        verts.insert(verts.end(), { 0, -halfH, 0, 0, -1, 0, 0.5f, 0.5f }); // center
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * (float)M_PI;
            float x = radius * cosf(angle);
            float z = radius * sinf(angle);
            float u = 0.5f + 0.5f * cosf(angle);
            float v = 0.5f - 0.5f * sinf(angle);
            verts.insert(verts.end(), { x, -halfH, z, 0, -1, 0, u, v });
        }
        for (int i = 0; i < segments; i++) {
            indices.insert(indices.end(), { botCenter, botCenter + 2 + (uint32_t)i, botCenter + 1 + (uint32_t)i });
        }

        auto vao = Heron::VertexArray::Create();
        auto vbo = Heron::VertexBuffer::Create(verts.data(), (uint32_t)(verts.size() * sizeof(float)));
        vbo->SetLayout({
            { Heron::ShaderDataType::Float3, "a_Position" },
            { Heron::ShaderDataType::Float3, "a_Normal" },
            { Heron::ShaderDataType::Float2, "a_TexCoord" }
        });
        vao->AddVertexBuffer(vbo);
        auto ibo = Heron::IndexBuffer::Create(indices.data(), (uint32_t)indices.size());
        vao->SetIndexBuffer(ibo);

        m_PrimitiveMeshes[(int)Heron::PrimitiveType::Cylinder] = vao;
    }
}

// ============================================================
//  INIT GL
// ============================================================
void ViewportWidget::initializeGL()
{
    Heron::RenderCommand::Init();

	if (Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan) {
		// Initialize Vulkan surface using HWND
		HWND hwnd = (HWND)this->winId();
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = hwnd;
		createInfo.hinstance = GetModuleHandle(nullptr);

		if (vkCreateWin32SurfaceKHR(Heron::VulkanContext::Get()->GetInstance(), &createInfo, nullptr, &m_VulkanSurface) != VK_SUCCESS) {
			HRN_CORE_ERROR("Failed to create Vulkan Win32 Surface!");
		}

		m_VulkanSwapchain = std::make_shared<Heron::VulkanSwapchain>(m_VulkanSurface, width(), height());
	} else {
		int status = gladLoadGL((GLADloadfunc)QtGLGetProcAddress);
		if (!status) {
			HRN_CORE_ERROR("Failed to initialize GLAD inside QOpenGLWidget!");
		} else {
			HRN_CORE_INFO("GLAD initialized in Qt Viewport.");
		}
	}

    uint32_t whiteTextureData = 0xffffffff;
    m_WhiteTexture = Heron::Texture2D::Create(1, 1);
    m_WhiteTexture->Bind();

    // ===== Grid Setup =====
    m_GridVAO = Heron::VertexArray::Create();
    float quadVerts[4 * 3] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    auto vbo = Heron::VertexBuffer::Create(quadVerts, sizeof(quadVerts));
    vbo->SetLayout({ { Heron::ShaderDataType::Float3, "a_Position" } });
    m_GridVAO->AddVertexBuffer(vbo);
    uint32_t quadIdx[6] = { 0, 1, 2, 2, 3, 0 };
    auto ibo = Heron::IndexBuffer::Create(quadIdx, 6);
    m_GridVAO->SetIndexBuffer(ibo);

    // ===== Gizmo Setup =====
    m_GizmoShader = Heron::Shader::Create("assets/shaders/Gizmo.vert.spv", "assets/shaders/Gizmo.frag.spv");

    m_GizmoLineVAO = Heron::VertexArray::Create();
    float lineVerts[] = { 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f };
    auto lineVBO = Heron::VertexBuffer::Create(lineVerts, sizeof(lineVerts));
    lineVBO->SetLayout({ { Heron::ShaderDataType::Float3, "a_Position" } });
    m_GizmoLineVAO->AddVertexBuffer(lineVBO);
    uint32_t lineIdx[] = { 0, 1 };
    m_GizmoLineVAO->SetIndexBuffer(Heron::IndexBuffer::Create(lineIdx, 2));

    m_GizmoCircleVAO = Heron::VertexArray::Create();
    const int numSegments = 32;
    std::vector<float> circleVerts;
    std::vector<uint32_t> circleIdx;
    for (int i = 0; i < numSegments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(numSegments);
        circleVerts.push_back(cosf(theta));
        circleVerts.push_back(sinf(theta));
        circleVerts.push_back(0.0f);
        
        // Generate LINE_LIST indices
        circleIdx.push_back(i);
        circleIdx.push_back((i + 1) % numSegments);
    }
    auto circleVBO = Heron::VertexBuffer::Create(circleVerts.data(), circleVerts.size() * sizeof(float));
    circleVBO->SetLayout({ { Heron::ShaderDataType::Float3, "a_Position" } });
    m_GizmoCircleVAO->AddVertexBuffer(circleVBO);
    m_GizmoCircleVAO->SetIndexBuffer(Heron::IndexBuffer::Create(circleIdx.data(), circleIdx.size()));

    // ===== Skybox Setup =====
    CreateSkyboxShader();
    m_SkyboxVAO = Heron::VertexArray::Create();
    float skyboxVerts[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
    };
    uint32_t skyboxIdx[] = {
        0, 1, 2, 2, 3, 0, // front
        4, 5, 7, 7, 6, 4, // back
        5, 0, 3, 3, 7, 5, // top
        1, 4, 6, 6, 2, 1, // bottom
        5, 4, 1, 1, 0, 5, // left
        3, 2, 6, 6, 7, 3  // right
    };
    auto skyboxVBO = Heron::VertexBuffer::Create(skyboxVerts, sizeof(skyboxVerts));
    skyboxVBO->SetLayout({ { Heron::ShaderDataType::Float3, "a_Position" } });
    m_SkyboxVAO->AddVertexBuffer(skyboxVBO);
    m_SkyboxVAO->SetIndexBuffer(Heron::IndexBuffer::Create(skyboxIdx, 36));

    // Grid shader (infinite grid via ray-plane intersection)
    std::string gridVert = R"(
        #version 330 core
        layout(location = 0) in vec3 a_Position;
        uniform mat4 u_InvView;
        uniform mat4 u_InvProj;
        out vec3 v_NearPoint;
        out vec3 v_FarPoint;

        vec3 UnprojectPoint(float x, float y, float z) {
            vec4 clipPoint = vec4(x, y, z, 1.0);
            vec4 viewPoint = u_InvProj * clipPoint;
            viewPoint = viewPoint / viewPoint.w;
            vec4 worldPoint = u_InvView * viewPoint;
            return worldPoint.xyz / worldPoint.w;
        }

        void main() {
            v_NearPoint = UnprojectPoint(a_Position.x, a_Position.y, 0.0);
            v_FarPoint  = UnprojectPoint(a_Position.x, a_Position.y, 1.0);
            gl_Position = vec4(a_Position, 1.0);
        }
    )";

    std::string gridFrag = R"(
        #version 330 core
        in vec3 v_NearPoint;
        in vec3 v_FarPoint;
        uniform mat4 u_View;
        uniform mat4 u_Proj;
        uniform float u_Near;
        uniform float u_Far;
        layout(location = 0) out vec4 FragColor;

        float ComputeDepth(vec3 pos) {
            vec4 clipPos = u_Proj * u_View * vec4(pos, 1.0);
            return (clipPos.z / clipPos.w) * 0.5 + 0.5;
        }

        float ComputeLinearDepth(vec3 pos) {
            vec4 clipPos = u_Proj * u_View * vec4(pos, 1.0);
            float clipDepth = (clipPos.z / clipPos.w) * 2.0 - 1.0;
            float linearDepth = (2.0 * u_Near * u_Far) / (u_Far + u_Near - clipDepth * (u_Far - u_Near));
            return linearDepth / u_Far;
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
    )";

	if (Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan) {
		m_GridShader = Heron::Shader::Create("assets/shaders/Grid.vert.spv", "assets/shaders/Grid.frag.spv");
	} else {
		m_GridShader = Heron::Shader::Create(gridVert, gridFrag);
	}

    // ===== Mesh Shader + Primitives =====
    CreateMeshShader();
    GeneratePrimitiveMeshes();
}

// ============================================================
//  RENDER ENTITIES
// ============================================================
void ViewportWidget::RenderEntities()
{
    if (!m_ActiveScene) return;

    glm::mat4 viewProj = m_Camera.GetViewProjection();

    if (m_SceneState == SceneState::Play) {
        auto cameraView = m_ActiveScene->GetRegistry().view<Heron::TransformComponent, Heron::CameraComponent>();
        for (auto entity : cameraView) {
            auto [transform, camera] = cameraView.get<Heron::TransformComponent, Heron::CameraComponent>(entity);
            if (camera.Primary) {
                float aspect = (float)width() / (float)height();
                glm::mat4 projection = glm::perspective(glm::radians(camera.FOV), aspect, camera.NearClip, camera.FarClip);
                if (Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan) {
                    projection[2][2] = projection[2][2] * 0.5f + projection[2][3] * 0.5f;
                    projection[3][2] = projection[3][2] * 0.5f + projection[3][3] * 0.5f;
                }

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(transform.Translation[0], transform.Translation[1], transform.Translation[2]));
                model = glm::rotate(model, transform.Rotation[0], glm::vec3(1, 0, 0));
                model = glm::rotate(model, transform.Rotation[1], glm::vec3(0, 1, 0));
                model = glm::rotate(model, transform.Rotation[2], glm::vec3(0, 0, 1));
                
                glm::mat4 view = glm::inverse(model);
                viewProj = projection * view;
                break;
            }
        }
    }

    if (Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan) {
        if (m_VulkanSwapchain && m_PrimitiveMeshes.count((int)Heron::PrimitiveType::Cube)) {
            auto vkShader = std::static_pointer_cast<Heron::VulkanShader>(m_MeshShader);
            auto pass = m_VulkanSwapchain->GetRenderPass();
            auto layout = m_PrimitiveMeshes[(int)Heron::PrimitiveType::Cube]->GetVertexBuffers()[0]->GetLayout();
            vkShader->BuildPipeline(pass, layout, true, false);
        }
    }

    m_MeshShader->Bind();
    m_MeshShader->SetMat4("u_ViewProjection", viewProj);

    auto& registry = m_ActiveScene->GetRegistry();

    // Find the first directional light (shared by both OpenGL and Vulkan paths)
    bool hasLight = false;
    glm::vec3 lightDir(0.0f);
    glm::vec3 lightColor(1.0f);
    float lightIntensity = 1.0f;

    auto lightView = registry.view<Heron::TransformComponent, Heron::LightComponent>();
    for (auto entity : lightView) {
        auto [transform, light] = lightView.get<Heron::TransformComponent, Heron::LightComponent>(entity);
        
        glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), transform.Rotation[0], glm::vec3(1, 0, 0))
                         * glm::rotate(glm::mat4(1.0f), transform.Rotation[1], glm::vec3(0, 1, 0))
                         * glm::rotate(glm::mat4(1.0f), transform.Rotation[2], glm::vec3(0, 0, 1));
        lightDir = glm::normalize(glm::vec3(rotMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
        lightColor = glm::vec3(light.Color[0], light.Color[1], light.Color[2]);
        lightIntensity = light.Intensity;
        hasLight = true;
        break; // Only support one directional light for now
    }

    // OpenGL path: set uniforms individually
    m_MeshShader->SetInt("u_HasLight", hasLight ? 1 : 0);
    if (hasLight) {
        m_MeshShader->SetFloat3("u_LightDir", lightDir);
        m_MeshShader->SetFloat3("u_LightColor", lightColor);
        m_MeshShader->SetFloat("u_LightIntensity", lightIntensity);
    }

    // Vulkan path: upload all scene data to UBO in one shot
    if (Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan) {
        Heron::VulkanShader::SceneData sceneData{};
        sceneData.ViewProjection = viewProj;
        sceneData.HasLight = hasLight ? 1 : 0;
        if (hasLight) {
            sceneData.LightDir = glm::vec4(lightDir, 0.0f);
            sceneData.LightColor = glm::vec4(lightColor, 0.0f);
            sceneData.LightIntensity = lightIntensity;
        }
        if (m_MeshShader) std::static_pointer_cast<Heron::VulkanShader>(m_MeshShader)->SetSceneData(sceneData);
        if (m_GizmoShader) std::static_pointer_cast<Heron::VulkanShader>(m_GizmoShader)->SetSceneData(sceneData);
        if (m_GridShader) std::static_pointer_cast<Heron::VulkanShader>(m_GridShader)->SetSceneData(sceneData);
    }

    auto view = registry.view<Heron::TransformComponent, Heron::MeshComponent>();

    for (auto entity : view) {
        auto& transform = view.get<Heron::TransformComponent>(entity);
        auto& mesh = view.get<Heron::MeshComponent>(entity);

        // Build model matrix from transform
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(
            transform.Translation[0],
            transform.Translation[1],
            transform.Translation[2]
        ));
        model = glm::rotate(model, transform.Rotation[0], glm::vec3(1, 0, 0));
        model = glm::rotate(model, transform.Rotation[1], glm::vec3(0, 1, 0));
        model = glm::rotate(model, transform.Rotation[2], glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(transform.Scale[0], transform.Scale[1], transform.Scale[2]));

        m_MeshShader->SetMat4("u_Model", model);
        m_MeshShader->SetFloat4("u_Color", glm::vec4(
            mesh.Color[0], mesh.Color[1], mesh.Color[2], mesh.Color[3]
        ));
        
        if (Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan) {
            auto pass = m_VulkanSwapchain->GetRenderPass();
            auto vkShader = std::static_pointer_cast<Heron::VulkanShader>(m_MeshShader);
            vkShader->BuildPipeline(pass, m_PrimitiveMeshes[(int)mesh.Type]->GetVertexBuffers()[0]->GetLayout(), true, false);
        }

        if (mesh.Texture) {
            mesh.Texture->Bind(0);
        } else if (m_WhiteTexture) {
            m_WhiteTexture->Bind(0);
        }
        m_MeshShader->SetInt("u_Texture", 0);

        auto it = m_PrimitiveMeshes.find((int)mesh.Type);
        if (it != m_PrimitiveMeshes.end()) {
            it->second->Bind();
            if (Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan) {
                // In Vulkan, we push constants *after* binding the pipeline but before drawing
                std::static_pointer_cast<Heron::VulkanShader>(m_MeshShader)->Bind();
            }
            Heron::RenderCommand::DrawIndexed(it->second);
        }
    }

    // --- Skybox Rendering ---
    bool isVulkanRender = Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan;
    auto skyboxView = registry.view<Heron::SkyboxComponent>();
    for (auto entity : skyboxView) {
        auto& sc = skyboxView.get<Heron::SkyboxComponent>(entity);
        if (sc.Texture) {
            if (!isVulkanRender) {
                glDepthFunc(GL_LEQUAL); // Render skybox behind everything
            }

            if (isVulkanRender && m_VulkanSwapchain) {
                auto vkShader = std::static_pointer_cast<Heron::VulkanShader>(m_SkyboxShader);
                vkShader->BuildPipeline(m_VulkanSwapchain->GetRenderPass(), m_SkyboxVAO->GetVertexBuffers()[0]->GetLayout(), true, false);
            }

            m_SkyboxShader->Bind();
            
            // Extract projection and ONLY the rotation part of the view matrix
            glm::mat4 view = glm::mat4(1.0f);
            glm::mat4 projection = glm::mat4(1.0f);
            
            if (m_SceneState == SceneState::Play) {
                auto cameraView = m_ActiveScene->GetRegistry().view<Heron::TransformComponent, Heron::CameraComponent>();
                for (auto camEnt : cameraView) {
                    auto [transform, camera] = cameraView.get<Heron::TransformComponent, Heron::CameraComponent>(camEnt);
                    if (camera.Primary) {
                        float aspect = (float)width() / (float)height();
                        projection = glm::perspective(glm::radians(camera.FOV), aspect, camera.NearClip, camera.FarClip);
                        
                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, glm::vec3(transform.Translation[0], transform.Translation[1], transform.Translation[2]));
                        model = glm::rotate(model, transform.Rotation[0], glm::vec3(1, 0, 0));
                        model = glm::rotate(model, transform.Rotation[1], glm::vec3(0, 1, 0));
                        model = glm::rotate(model, transform.Rotation[2], glm::vec3(0, 0, 1));
                        
                        view = glm::inverse(model);
                        break;
                    }
                }
            } else {
                projection = m_Camera.GetProjection();
                view = m_Camera.GetViewMatrix();
            }

            // Remove translation from view matrix
            view = glm::mat4(glm::mat3(view));
            
            if (isVulkanRender) {
                Heron::VulkanShader::SceneData skyboxSceneData{};
                skyboxSceneData.ViewProjection = projection * view;
                std::static_pointer_cast<Heron::VulkanShader>(m_SkyboxShader)->SetSceneData(skyboxSceneData);
            } else {
                m_SkyboxShader->SetMat4("u_View", view);
                m_SkyboxShader->SetMat4("u_Projection", projection);
            }
            
            sc.Texture->Bind(0);
            m_SkyboxShader->SetInt("u_Skybox", 0);
            
            m_SkyboxVAO->Bind();
            if (isVulkanRender) {
                std::static_pointer_cast<Heron::VulkanShader>(m_SkyboxShader)->Bind();
                Heron::RenderCommand::DrawIndexed(m_SkyboxVAO);
            } else {
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            }
            
            if (!isVulkanRender) {
                glDepthFunc(GL_LESS); // Restore default
            }
            break; // Only render one skybox
        }
    }
}

// ============================================================
//  RESIZE / PAINT
// ============================================================
void ViewportWidget::resizeGL(int w, int h)
{
    if (Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan) {
        if (m_VulkanSwapchain) m_VulkanSwapchain->OnResize(w, h);
    } else {
        glViewport(0, 0, w, h);
    }
    m_Camera.OnResize(w, h);
}

void ViewportWidget::resizeEvent(QResizeEvent* event)
{
    if (m_Initialized) {
        resizeGL(event->size().width(), event->size().height());
    }
    QWidget::resizeEvent(event);
}

void ViewportWidget::paintEvent(QPaintEvent* event)
{
    if (!m_Initialized) {
        initializeGL();
        resizeGL(width(), height());
        m_Initialized = true;
    }
    paintGL();
}

void ViewportWidget::paintGL()
{
	bool isVulkan = Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan;

	if (isVulkan) {
		if (m_VulkanSwapchain) {
			if (!m_VulkanSwapchain->BeginFrame()) return;
		} else {
			return;
		}
	} else {
		Heron::RenderCommand::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		Heron::RenderCommand::Clear();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
	}

	// 1) Draw infinite grid (only in Edit mode)
	if (m_SceneState == SceneState::Edit) {
		if (isVulkan && m_VulkanSwapchain) {
			auto vkShader = std::static_pointer_cast<Heron::VulkanShader>(m_GridShader);
			vkShader->BuildPipeline(m_VulkanSwapchain->GetRenderPass(), m_GridVAO->GetVertexBuffers()[0]->GetLayout(), true, false);
		}

		// Bind dummy texture to slot 0 for Vulkan descriptor set validation
		if (m_WhiteTexture) m_WhiteTexture->Bind(0);

		// OpenGL path: pass projection and view separately via uniforms
		if (!isVulkan) {
			m_GridShader->SetMat4("u_ViewProjection", m_Camera.GetProjection());
			m_GridShader->SetMat4("u_Model", m_Camera.GetViewMatrix());
		}
		// Vulkan path: Grid reads u_ViewProjection from SceneData UBO (set in RenderEntities)
		
		m_GridVAO->Bind();
		m_GridShader->Bind();

		Heron::RenderCommand::DrawIndexed(m_GridVAO);
	}

	// 2) Draw all spawned mesh entities
	RenderEntities();

	// 3) Draw Gizmo
	DrawGizmo();

	if (isVulkan) {
		m_VulkanSwapchain->EndFrame();
	} else {
		glDisable(GL_BLEND);
	}
}

// ============================================================
//  GAME LOOP
// ============================================================
void ViewportWidget::SetSceneState(SceneState state)
{
    HRN_CORE_INFO("ViewportWidget::SetSceneState -> {0}", (int)state);
    if (m_SceneState == state)
        return;

    m_SceneState = state;

    if (m_SceneState == SceneState::Play) {
        HRN_CORE_INFO("Copying Editor Scene to Active Scene...");
        m_EditorScene = m_ActiveScene;
        m_ActiveScene = Heron::Scene::Copy(m_EditorScene);
        HRN_CORE_INFO("Copy complete.");
        
        m_ScoreLabel->setText("Score: 0");
        m_ScoreLabel->show();
    } else {
        m_ActiveScene = m_EditorScene;
        m_ScoreLabel->hide();
    }

    emit SceneChanged(m_ActiveScene.get());
    emit EntitySpawned(); // Quick hack to refresh hierarchy and properties
}

void ViewportWidget::updateGameLoop()
{
    if (m_ActiveScene) {
        if (m_SceneState == SceneState::Play) {
            HRN_CORE_INFO("updateGameLoop -> OnUpdateRuntime START");
            m_ActiveScene->OnUpdateRuntime(0.016f);
            HRN_CORE_INFO("updateGameLoop -> OnUpdateRuntime END");
        }
        else
            m_ActiveScene->OnUpdateEditor(0.016f);
    }
    update();
}

// ============================================================
//  CONTEXT MENU — Right-click to spawn primitives
// ============================================================
void ViewportWidget::ShowContextMenu(const QPoint& pos)
{
    QMenu menu(this);

    QAction* cubeAction = menu.addAction("Add Cube");
    QAction* planeAction = menu.addAction("Add Plane");
    QAction* cylinderAction = menu.addAction("Add Cylinder");

    menu.addSeparator();
    QAction* cameraAction = menu.addAction("Add Camera");
    QAction* emptyAction = menu.addAction("Add Empty Entity");

    connect(cubeAction, &QAction::triggered, this, &ViewportWidget::spawnCube);
    connect(planeAction, &QAction::triggered, this, &ViewportWidget::spawnPlane);
    connect(cylinderAction, &QAction::triggered, this, &ViewportWidget::spawnCylinder);
    connect(cameraAction, &QAction::triggered, this, [this]() {
        m_SpawnCounter++;
        auto entity = m_ActiveScene->CreateEntity("Camera_" + std::to_string(m_SpawnCounter));
        
        // Add camera component
        auto& cam = entity.AddComponent<Heron::CameraComponent>();
        cam.Primary = true;
        cam.FOV = 45.0f;
        cam.NearClip = 0.1f;
        cam.FarClip = 1000.0f;
        
        // Align to current editor viewport camera
        auto& transform = entity.GetComponent<Heron::TransformComponent>();
        glm::vec3 camPos = m_Camera.GetPosition();
        transform.Translation[0] = camPos.x;
        transform.Translation[1] = camPos.y;
        transform.Translation[2] = camPos.z;
        
        // Set rotation from editor camera pitch/yaw
        transform.Rotation[0] = -m_Camera.GetPitch();  // Pitch (X rotation)
        transform.Rotation[1] = -m_Camera.GetYaw();    // Yaw (Y rotation)
        transform.Rotation[2] = 0.0f;                   // No roll
        
        emit EntitySpawned();
    });
    connect(emptyAction, &QAction::triggered, this, [this]() {
        m_SpawnCounter++;
        m_ActiveScene->CreateEntity("Empty_" + std::to_string(m_SpawnCounter));
    });

    menu.exec(mapToGlobal(pos));
}

void ViewportWidget::spawnCube()
{
    m_SpawnCounter++;
    auto entity = m_ActiveScene->CreateEntity("Cube_" + std::to_string(m_SpawnCounter));
    auto& mesh = entity.AddComponent<Heron::MeshComponent>(Heron::PrimitiveType::Cube);
    mesh.Color[0] = 0.6f; mesh.Color[1] = 0.65f; mesh.Color[2] = 0.75f; mesh.Color[3] = 1.0f;
    emit EntitySpawned();
}

void ViewportWidget::spawnPlane()
{
    m_SpawnCounter++;
    auto entity = m_ActiveScene->CreateEntity("Plane_" + std::to_string(m_SpawnCounter));
    auto& mesh = entity.AddComponent<Heron::MeshComponent>(Heron::PrimitiveType::Plane);
    mesh.Color[0] = 0.45f; mesh.Color[1] = 0.55f; mesh.Color[2] = 0.45f; mesh.Color[3] = 1.0f;
    emit EntitySpawned();
}

void ViewportWidget::spawnCylinder()
{
    m_SpawnCounter++;
    auto entity = m_ActiveScene->CreateEntity("Cylinder_" + std::to_string(m_SpawnCounter));
    auto& mesh = entity.AddComponent<Heron::MeshComponent>(Heron::PrimitiveType::Cylinder);
    mesh.Color[0] = 0.7f; mesh.Color[1] = 0.5f; mesh.Color[2] = 0.4f; mesh.Color[3] = 1.0f;
    emit EntitySpawned();
}

// ============================================================
//  MOUSE INPUT — Pan, Rotate, Zoom + Right-click context menu
// ============================================================
void ViewportWidget::mousePressEvent(QMouseEvent* event)
{
    m_LastMousePosition = event->position();
    m_RightClickDragged = false;
    setFocus();

    if (event->button() == Qt::LeftButton && m_GizmoMode != GizmoMode::None && m_SelectedEntity) {
        auto& transform = m_SelectedEntity.GetComponent<Heron::TransformComponent>();
        glm::vec3 objPos(transform.Translation[0], transform.Translation[1], transform.Translation[2]);
        
        glm::vec3 rayOrigin, rayDir;
        ScreenToWorldRay(event->position().x(), event->position().y(), rayOrigin, rayDir);

        m_ActiveGizmoAxis = -1;
        float bestDist = 9999.0f;
        float hitT = 0.0f;
        
        float handleSize = 1.5f;

        if (m_GizmoMode == GizmoMode::Translate) {
            // X Axis
            float tX; float dX = RayLineDistance(rayOrigin, rayDir, objPos, glm::vec3(1,0,0), tX);
            if (dX < 0.2f && tX > 0 && tX < handleSize && dX < bestDist) { bestDist = dX; m_ActiveGizmoAxis = 0; hitT = tX; }
            // Y Axis
            float tY; float dY = RayLineDistance(rayOrigin, rayDir, objPos, glm::vec3(0,1,0), tY);
            if (dY < 0.2f && tY > 0 && tY < handleSize && dY < bestDist) { bestDist = dY; m_ActiveGizmoAxis = 1; hitT = tY; }
            // Z Axis
            float tZ; float dZ = RayLineDistance(rayOrigin, rayDir, objPos, glm::vec3(0,0,1), tZ);
            if (dZ < 0.2f && tZ > 0 && tZ < handleSize && dZ < bestDist) { bestDist = dZ; m_ActiveGizmoAxis = 2; hitT = tZ; }
        }
        else if (m_GizmoMode == GizmoMode::Scale) {
            // Uniform Scale diagonal axis (1,1,1)
            glm::vec3 scaleDir = glm::normalize(glm::vec3(1,1,1));
            float tS; float dS = RayLineDistance(rayOrigin, rayDir, objPos, scaleDir, tS);
            if (dS < 0.3f && tS > 0 && tS < handleSize*1.5f) {
                m_ActiveGizmoAxis = 3;
                hitT = tS;
            }
        }
        else if (m_GizmoMode == GizmoMode::Rotate) {
            // Rings on X, Y, Z planes
            float radius = 1.2f;
            float tX, tY, tZ;
            if (RayPlaneIntersection(rayOrigin, rayDir, objPos, glm::vec3(1,0,0), tX)) {
                glm::vec3 hitPoint = rayOrigin + rayDir * tX;
                float distToRing = std::abs(glm::length(hitPoint - objPos) - radius);
                if (distToRing < 0.2f && distToRing < bestDist) { bestDist = distToRing; m_ActiveGizmoAxis = 0; hitT = tX; }
            }
            if (RayPlaneIntersection(rayOrigin, rayDir, objPos, glm::vec3(0,1,0), tY)) {
                glm::vec3 hitPoint = rayOrigin + rayDir * tY;
                float distToRing = std::abs(glm::length(hitPoint - objPos) - radius);
                if (distToRing < 0.2f && distToRing < bestDist) { bestDist = distToRing; m_ActiveGizmoAxis = 1; hitT = tY; }
            }
            if (RayPlaneIntersection(rayOrigin, rayDir, objPos, glm::vec3(0,0,1), tZ)) {
                glm::vec3 hitPoint = rayOrigin + rayDir * tZ;
                float distToRing = std::abs(glm::length(hitPoint - objPos) - radius);
                if (distToRing < 0.2f && distToRing < bestDist) { bestDist = distToRing; m_ActiveGizmoAxis = 2; hitT = tZ; }
            }
        }

        if (m_ActiveGizmoAxis != -1) {
            m_GizmoDragging = true;
            m_DragStartPoint = rayOrigin + rayDir * hitT;
            m_DragStartEntityPos = objPos;
            m_DragStartEntityRot = glm::vec3(transform.Rotation[0], transform.Rotation[1], transform.Rotation[2]);
            m_DragStartEntityScale = glm::vec3(transform.Scale[0], transform.Scale[1], transform.Scale[2]);
            return; // Consume event
        }
    }
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_GizmoDragging && event->button() == Qt::LeftButton) {
        m_GizmoDragging = false;
        m_ActiveGizmoAxis = -1;
    }

    // Show context menu only if right-click released without dragging
    if (event->button() == Qt::RightButton && !m_RightClickDragged) {
        ShowContextMenu(event->pos());
    }
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* event)
{
    QPointF currentPosition = event->position();
    glm::vec2 delta = {
        (float)(currentPosition.x() - m_LastMousePosition.x()),
        (float)(currentPosition.y() - m_LastMousePosition.y())
    };

    // Check if the mouse actually moved (dead zone of 2px to avoid accidental drags)
    if (glm::length(delta) > 2.0f && (event->buttons() & Qt::RightButton)) {
        m_RightClickDragged = true;
    }

    if (m_GizmoDragging && m_SelectedEntity) {
        glm::vec3 rayOrigin, rayDir;
        ScreenToWorldRay(currentPosition.x(), currentPosition.y(), rayOrigin, rayDir);

        auto& transform = m_SelectedEntity.GetComponent<Heron::TransformComponent>();

        if (m_GizmoMode == GizmoMode::Translate) {
            glm::vec3 axis(0.0f);
            if (m_ActiveGizmoAxis == 0) axis = glm::vec3(1,0,0);
            if (m_ActiveGizmoAxis == 1) axis = glm::vec3(0,1,0);
            if (m_ActiveGizmoAxis == 2) axis = glm::vec3(0,0,1);

            float t;
            RayLineDistance(rayOrigin, rayDir, m_DragStartEntityPos, axis, t);
            
            glm::vec3 projectedPoint = m_DragStartEntityPos + axis * t;
            glm::vec3 deltaTrans = projectedPoint - m_DragStartPoint;
            
            transform.Translation[m_ActiveGizmoAxis] = SnapToIncrement(m_DragStartEntityPos[m_ActiveGizmoAxis] + deltaTrans[m_ActiveGizmoAxis], 0.01f);
        }
        else if (m_GizmoMode == GizmoMode::Scale) {
            glm::vec3 scaleDir = glm::normalize(glm::vec3(1,1,1));
            float t;
            RayLineDistance(rayOrigin, rayDir, m_DragStartEntityPos, scaleDir, t);
            
            glm::vec3 projectedPoint = m_DragStartEntityPos + scaleDir * t;
            float dragDist = glm::dot(projectedPoint - m_DragStartPoint, scaleDir);
            
            // Multiply dragDist to make it less sensitive
            float scaleDelta = SnapToIncrement(dragDist * 0.5f, 0.001f);
            
            transform.Scale[0] = m_DragStartEntityScale.x + scaleDelta;
            transform.Scale[1] = m_DragStartEntityScale.y + scaleDelta;
            transform.Scale[2] = m_DragStartEntityScale.z + scaleDelta;
        }
        else if (m_GizmoMode == GizmoMode::Rotate) {
            glm::vec3 axis(0.0f);
            if (m_ActiveGizmoAxis == 0) axis = glm::vec3(1,0,0);
            if (m_ActiveGizmoAxis == 1) axis = glm::vec3(0,1,0);
            if (m_ActiveGizmoAxis == 2) axis = glm::vec3(0,0,1);

            float t;
            if (RayPlaneIntersection(rayOrigin, rayDir, m_DragStartEntityPos, axis, t)) {
                glm::vec3 hitPoint = rayOrigin + rayDir * t;
                glm::vec3 currentVec = glm::normalize(hitPoint - m_DragStartEntityPos);
                glm::vec3 startVec = glm::normalize(m_DragStartPoint - m_DragStartEntityPos);
                
                float angle = acos(glm::clamp(glm::dot(startVec, currentVec), -1.0f, 1.0f));
                
                // Determine sign of angle using cross product
                glm::vec3 cross = glm::cross(startVec, currentVec);
                if (glm::dot(cross, axis) < 0) {
                    angle = -angle;
                }
                
                // Snap to integer degrees
                float newAngleDeg = glm::degrees(m_DragStartEntityRot[m_ActiveGizmoAxis] + angle);
                newAngleDeg = SnapToIncrement(newAngleDeg, 1.0f);
                
                transform.Rotation[m_ActiveGizmoAxis] = glm::radians(newAngleDeg);
            }
        }
        
        emit PropertiesChanged();
        update();
        m_LastMousePosition = currentPosition;
        return;
    }

    bool altHeld = (event->modifiers() & Qt::AltModifier);

    if (event->buttons() & Qt::MiddleButton) {
        m_Camera.MousePan(delta);
    }
    else if (altHeld && (event->buttons() & Qt::LeftButton)) {
        m_Camera.MousePan(delta);
    }
    else if ((event->buttons() & Qt::RightButton) && m_RightClickDragged) {
        m_Camera.MouseRotate(delta);
    }

    m_LastMousePosition = currentPosition;
    update();
}

void ViewportWidget::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().y() * 0.01f;
    m_Camera.MouseZoom(delta);
    event->accept();
    update();
}

// ============================================================
//  MATH HELPERS & GIZMO DRAWING
// ============================================================

void ViewportWidget::ScreenToWorldRay(int mouseX, int mouseY, glm::vec3& outOrigin, glm::vec3& outDir)
{
    float x = (2.0f * mouseX) / width() - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / height();
    float z = 1.0f;

    glm::vec3 ray_nds = glm::vec3(x, y, z);
    glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);

    float aspect = (float)width() / (float)height();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f); // Editor camera
    glm::mat4 view = m_Camera.GetViewMatrix();

    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

    glm::vec3 ray_wor = glm::vec3(glm::inverse(view) * ray_eye);
    ray_wor = glm::normalize(ray_wor);

    outOrigin = m_Camera.GetPosition();
    outDir = ray_wor;
}

float ViewportWidget::RayLineDistance(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& lineOrigin, const glm::vec3& lineDir, float& tLine)
{
    glm::vec3 w0 = rayOrigin - lineOrigin;
    float a = glm::dot(rayDir, rayDir);
    float b = glm::dot(rayDir, lineDir);
    float c = glm::dot(lineDir, lineDir);
    float d = glm::dot(rayDir, w0);
    float e = glm::dot(lineDir, w0);

    float denom = a * c - b * b;
    if (denom < 1e-6f) { // Parallel
        tLine = 0;
        return glm::length(w0 - rayDir * glm::dot(rayDir, w0));
    }

    float sc = (b * e - c * d) / denom;
    float tc = (a * e - b * d) / denom;

    tLine = tc;
    glm::vec3 pRay = rayOrigin + rayDir * sc;
    glm::vec3 pLine = lineOrigin + lineDir * tc;
    return glm::length(pRay - pLine);
}

bool ViewportWidget::RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& planeOrigin, const glm::vec3& planeNormal, float& t)
{
    float denom = glm::dot(planeNormal, rayDir);
    if (std::abs(denom) > 1e-6) {
        glm::vec3 p0l0 = planeOrigin - rayOrigin;
        t = glm::dot(p0l0, planeNormal) / denom;
        return t >= 0;
    }
    return false;
}

float ViewportWidget::SnapToIncrement(float value, float increment)
{
    if (increment <= 0.0f) return value;
    return std::round(value / increment) * increment;
}

void ViewportWidget::DrawGizmo()
{
    if (m_GizmoMode == GizmoMode::None || !m_SelectedEntity) return;

    auto& transform = m_SelectedEntity.GetComponent<Heron::TransformComponent>();
    glm::vec3 objPos(transform.Translation[0], transform.Translation[1], transform.Translation[2]);

    bool isVulkan = Heron::RendererAPI::GetAPI() == Heron::RendererAPI::API::Vulkan;

    if (!isVulkan) {
        glDisable(GL_DEPTH_TEST);
        glLineWidth(2.0f);
    }

    if (isVulkan && m_VulkanSwapchain) {
        auto vkShader = std::static_pointer_cast<Heron::VulkanShader>(m_GizmoShader);
        vkShader->BuildPipeline(m_VulkanSwapchain->GetRenderPass(), m_GizmoLineVAO->GetVertexBuffers()[0]->GetLayout(), false, false, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    }
    
    m_GizmoShader->Bind();
    
    if (!isVulkan) {
        m_GizmoShader->SetMat4("u_ViewProjection", m_Camera.GetViewProjection());
    }

    float handleSize = 1.5f;

    if (m_GizmoMode == GizmoMode::Translate) {
        m_GizmoLineVAO->Bind();

        // X Axis (Red)
        glm::mat4 modelX = glm::translate(glm::mat4(1.0f), objPos) * glm::scale(glm::mat4(1.0f), glm::vec3(handleSize));
        m_GizmoShader->SetMat4("u_Model", modelX);
        m_GizmoShader->SetFloat4("u_Color", (m_ActiveGizmoAxis == 0) ? glm::vec4(1,1,0,1) : glm::vec4(1,0,0,1));
        Heron::RenderCommand::DrawLines(m_GizmoLineVAO);

        // Y Axis (Green)
        glm::mat4 modelY = glm::translate(glm::mat4(1.0f), objPos) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0,0,1)) * glm::scale(glm::mat4(1.0f), glm::vec3(handleSize));
        m_GizmoShader->SetMat4("u_Model", modelY);
        m_GizmoShader->SetFloat4("u_Color", (m_ActiveGizmoAxis == 1) ? glm::vec4(1,1,0,1) : glm::vec4(0,1,0,1));
        Heron::RenderCommand::DrawLines(m_GizmoLineVAO);

        // Z Axis (Blue)
        glm::mat4 modelZ = glm::translate(glm::mat4(1.0f), objPos) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0,1,0)) * glm::scale(glm::mat4(1.0f), glm::vec3(handleSize));
        m_GizmoShader->SetMat4("u_Model", modelZ);
        m_GizmoShader->SetFloat4("u_Color", (m_ActiveGizmoAxis == 2) ? glm::vec4(1,1,0,1) : glm::vec4(0,0,1,1));
        Heron::RenderCommand::DrawLines(m_GizmoLineVAO);
    }
    else if (m_GizmoMode == GizmoMode::Scale) {
        m_GizmoLineVAO->Bind();
        // Diagonal line (Yellow/White)
        // We draw a line from origin to (1,1,1) by rotating appropriately, or just scale/rotate the line.
        // The line VAO is (0,0,0) to (1,0,0). We need it to point to (1,1,1).
        // (1,1,1) is rotated 45 deg around Z, then rotated around Y.
        glm::vec3 scaleDir = glm::normalize(glm::vec3(1,1,1));
        glm::vec3 crossAxis = glm::normalize(glm::cross(glm::vec3(1,0,0), scaleDir));
        float angle = acos(glm::dot(glm::vec3(1,0,0), scaleDir));
        glm::mat4 modelScale = glm::translate(glm::mat4(1.0f), objPos) * glm::rotate(glm::mat4(1.0f), angle, crossAxis) * glm::scale(glm::mat4(1.0f), glm::vec3(handleSize*1.5f));
        
        m_GizmoShader->SetMat4("u_Model", modelScale);
        m_GizmoShader->SetFloat4("u_Color", (m_ActiveGizmoAxis == 3) ? glm::vec4(1,1,0,1) : glm::vec4(1,1,1,1));
        Heron::RenderCommand::DrawLines(m_GizmoLineVAO);
    }
    else if (m_GizmoMode == GizmoMode::Rotate) {
        m_GizmoCircleVAO->Bind();
        float radius = 1.2f;

        // X Ring (Red)
        glm::mat4 modelX = glm::translate(glm::mat4(1.0f), objPos) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0,1,0)) * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
        m_GizmoShader->SetMat4("u_Model", modelX);
        m_GizmoShader->SetFloat4("u_Color", (m_ActiveGizmoAxis == 0) ? glm::vec4(1,1,0,1) : glm::vec4(1,0,0,1));
        Heron::RenderCommand::DrawLines(m_GizmoCircleVAO);

        // Y Ring (Green)
        glm::mat4 modelY = glm::translate(glm::mat4(1.0f), objPos) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1,0,0)) * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
        m_GizmoShader->SetMat4("u_Model", modelY);
        m_GizmoShader->SetFloat4("u_Color", (m_ActiveGizmoAxis == 1) ? glm::vec4(1,1,0,1) : glm::vec4(0,1,0,1));
        Heron::RenderCommand::DrawLines(m_GizmoCircleVAO);

        // Z Ring (Blue)
        glm::mat4 modelZ = glm::translate(glm::mat4(1.0f), objPos) * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
        m_GizmoShader->SetMat4("u_Model", modelZ);
        m_GizmoShader->SetFloat4("u_Color", (m_ActiveGizmoAxis == 2) ? glm::vec4(1,1,0,1) : glm::vec4(0,0,1,1));
        Heron::RenderCommand::DrawLines(m_GizmoCircleVAO);
    }

    if (!isVulkan) {
        glEnable(GL_DEPTH_TEST);
        glLineWidth(1.0f);
    }
}
