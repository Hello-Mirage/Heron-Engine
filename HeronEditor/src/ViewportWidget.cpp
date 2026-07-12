#include <glad/gl.h>
#include "ViewportWidget.h"
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <Heron/Core/Log.h>
#include <Heron/Renderer/EditorCamera.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ViewportWidget::ViewportWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_ActiveScene = Heron::CreateRef<Heron::Scene>();

    connect(&m_Timer, &QTimer::timeout, this, &ViewportWidget::updateGameLoop);
    m_Timer.start(16);
}

ViewportWidget::~ViewportWidget()
{
}

void ViewportWidget::SetScene(Heron::Ref<Heron::Scene> scene)
{
    m_ActiveScene = scene;
    emit SceneChanged(m_ActiveScene.get());
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

        uniform mat4 u_ViewProjection;
        uniform mat4 u_Model;

        out vec3 v_Normal;
        out vec3 v_WorldPos;

        void main() {
            vec4 worldPos = u_Model * vec4(a_Position, 1.0);
            v_WorldPos = worldPos.xyz;
            v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
            gl_Position = u_ViewProjection * worldPos;
        }
    )";

    std::string fragSrc = R"(
        #version 330 core
        in vec3 v_Normal;
        in vec3 v_WorldPos;

        uniform vec4 u_Color;

        layout(location = 0) out vec4 FragColor;

        void main() {
            // Simple directional lighting from above-right
            vec3 lightDir = normalize(vec3(0.4, 0.8, 0.3));
            vec3 normal = normalize(v_Normal);
            float diff = max(dot(normal, lightDir), 0.0);
            float ambient = 0.25;
            float lighting = ambient + diff * 0.75;

            FragColor = vec4(u_Color.rgb * lighting, u_Color.a);
        }
    )";

    m_MeshShader = Heron::Shader::Create(vertSrc, fragSrc);
}

// ============================================================
//  PRIMITIVE MESH GENERATION — Cube, Plane, Cylinder
// ============================================================
void ViewportWidget::GeneratePrimitiveMeshes()
{
    // ---- CUBE (24 vertices with normals for flat shading) ----
    {
        float s = 0.5f;
        // pos(3) + normal(3) = 6 floats per vertex, 6 faces * 4 verts = 24 verts
        float cubeVerts[] = {
            // Front face (+Z)
            -s,-s, s,  0, 0, 1,   s,-s, s,  0, 0, 1,   s, s, s,  0, 0, 1,  -s, s, s,  0, 0, 1,
            // Back face (-Z)
             s,-s,-s,  0, 0,-1,  -s,-s,-s,  0, 0,-1,  -s, s,-s,  0, 0,-1,   s, s,-s,  0, 0,-1,
            // Top face (+Y)
            -s, s, s,  0, 1, 0,   s, s, s,  0, 1, 0,   s, s,-s,  0, 1, 0,  -s, s,-s,  0, 1, 0,
            // Bottom face (-Y)
            -s,-s,-s,  0,-1, 0,   s,-s,-s,  0,-1, 0,   s,-s, s,  0,-1, 0,  -s,-s, s,  0,-1, 0,
            // Right face (+X)
             s,-s, s,  1, 0, 0,   s,-s,-s,  1, 0, 0,   s, s,-s,  1, 0, 0,   s, s, s,  1, 0, 0,
            // Left face (-X)
            -s,-s,-s, -1, 0, 0,  -s,-s, s, -1, 0, 0,  -s, s, s, -1, 0, 0,  -s, s,-s, -1, 0, 0,
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
            { Heron::ShaderDataType::Float3, "a_Normal" }
        });
        vao->AddVertexBuffer(vbo);
        auto ibo = Heron::IndexBuffer::Create(cubeIdx, 36);
        vao->SetIndexBuffer(ibo);

        m_PrimitiveMeshes[(int)Heron::PrimitiveType::Cube] = vao;
    }

    // ---- PLANE (4 vertices, slightly above 0 to avoid being hidden by the grid) ----
    {
        float planeVerts[] = {
            // pos + normal
            -5.0f, 0.001f, -5.0f,  0, 1, 0,
             5.0f, 0.001f, -5.0f,  0, 1, 0,
             5.0f, 0.001f,  5.0f,  0, 1, 0,
            -5.0f, 0.001f,  5.0f,  0, 1, 0,
        };
        uint32_t planeIdx[] = { 0, 1, 2, 2, 3, 0 };

        auto vao = Heron::VertexArray::Create();
        auto vbo = Heron::VertexBuffer::Create(planeVerts, sizeof(planeVerts));
        vbo->SetLayout({
            { Heron::ShaderDataType::Float3, "a_Position" },
            { Heron::ShaderDataType::Float3, "a_Normal" }
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

            // Top vertex
            verts.insert(verts.end(), { x, halfH, z, nx, 0, nz });
            // Bottom vertex
            verts.insert(verts.end(), { x, -halfH, z, nx, 0, nz });
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
        uint32_t topCenter = (uint32_t)(verts.size() / 6);
        verts.insert(verts.end(), { 0, halfH, 0, 0, 1, 0 }); // center
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * (float)M_PI;
            float x = radius * cosf(angle);
            float z = radius * sinf(angle);
            verts.insert(verts.end(), { x, halfH, z, 0, 1, 0 });
        }
        for (int i = 0; i < segments; i++) {
            indices.insert(indices.end(), { topCenter, topCenter + 1 + (uint32_t)i, topCenter + 2 + (uint32_t)i });
        }

        // --- Bottom cap ---
        uint32_t botCenter = (uint32_t)(verts.size() / 6);
        verts.insert(verts.end(), { 0, -halfH, 0, 0, -1, 0 }); // center
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * (float)M_PI;
            float x = radius * cosf(angle);
            float z = radius * sinf(angle);
            verts.insert(verts.end(), { x, -halfH, z, 0, -1, 0 });
        }
        for (int i = 0; i < segments; i++) {
            indices.insert(indices.end(), { botCenter, botCenter + 2 + (uint32_t)i, botCenter + 1 + (uint32_t)i });
        }

        auto vao = Heron::VertexArray::Create();
        auto vbo = Heron::VertexBuffer::Create(verts.data(), (uint32_t)(verts.size() * sizeof(float)));
        vbo->SetLayout({
            { Heron::ShaderDataType::Float3, "a_Position" },
            { Heron::ShaderDataType::Float3, "a_Normal" }
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
    int status = gladLoadGL((GLADloadfunc)QtGLGetProcAddress);
    if (!status) {
        HRN_CORE_ERROR("Failed to initialize GLAD inside QOpenGLWidget!");
    } else {
        HRN_CORE_INFO("GLAD initialized in Qt Viewport.");
    }

    Heron::RenderCommand::Init();

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

    m_GridShader = Heron::Shader::Create(gridVert, gridFrag);

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

    m_MeshShader->Bind();
    m_MeshShader->SetMat4("u_ViewProjection", viewProj);

    auto& registry = m_ActiveScene->GetRegistry();
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
        model = glm::scale(model, glm::vec3(
            transform.Scale[0],
            transform.Scale[1],
            transform.Scale[2]
        ));

        m_MeshShader->SetMat4("u_Model", model);
        m_MeshShader->SetFloat4("u_Color", glm::vec4(
            mesh.Color[0], mesh.Color[1], mesh.Color[2], mesh.Color[3]
        ));

        auto it = m_PrimitiveMeshes.find((int)mesh.Type);
        if (it != m_PrimitiveMeshes.end()) {
            it->second->Bind();
            Heron::RenderCommand::DrawIndexed(it->second);
        }
    }
}

// ============================================================
//  RESIZE / PAINT
// ============================================================
void ViewportWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    m_Camera.OnResize(w, h);
}

void ViewportWidget::paintGL()
{
    Heron::RenderCommand::SetClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    Heron::RenderCommand::Clear();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // 1) Draw infinite grid (only in Edit mode)
    if (m_SceneState == SceneState::Edit) {
        m_GridShader->Bind();
        glm::mat4 invView = glm::inverse(m_Camera.GetViewMatrix());
        glm::mat4 invProj = glm::inverse(m_Camera.GetProjection());
        m_GridShader->SetMat4("u_InvView", invView);
        m_GridShader->SetMat4("u_InvProj", invProj);
        m_GridShader->SetMat4("u_View", m_Camera.GetViewMatrix());
        m_GridShader->SetMat4("u_Proj", m_Camera.GetProjection());
        m_GridShader->SetFloat("u_Near", 0.1f);
        m_GridShader->SetFloat("u_Far", 1000.0f);
        m_GridVAO->Bind();
        Heron::RenderCommand::DrawIndexed(m_GridVAO);
    }

    // 2) Draw all spawned mesh entities
    RenderEntities();

    glDisable(GL_BLEND);
}

// ============================================================
//  GAME LOOP
// ============================================================
void ViewportWidget::SetSceneState(SceneState state)
{
    if (m_SceneState == state)
        return;

    m_SceneState = state;

    if (m_SceneState == SceneState::Play) {
        m_EditorScene = m_ActiveScene;
        m_ActiveScene = Heron::Scene::Copy(m_EditorScene);
    } else {
        m_ActiveScene = m_EditorScene;
    }

    emit SceneChanged(m_ActiveScene.get());
    emit EntitySpawned(); // Quick hack to refresh hierarchy and properties
}

void ViewportWidget::updateGameLoop()
{
    if (m_ActiveScene) {
        if (m_SceneState == SceneState::Play)
            m_ActiveScene->OnUpdateRuntime(0.016f);
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
    QAction* emptyAction = menu.addAction("Add Empty Entity");

    connect(cubeAction, &QAction::triggered, this, &ViewportWidget::spawnCube);
    connect(planeAction, &QAction::triggered, this, &ViewportWidget::spawnPlane);
    connect(cylinderAction, &QAction::triggered, this, &ViewportWidget::spawnCylinder);
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
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent* event)
{
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
