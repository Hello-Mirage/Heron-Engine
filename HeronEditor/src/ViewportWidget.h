#pragma once

#include <QOpenGLWidget>
#include <QTimer>
#include <Heron/Heron.h>
#include <Heron/Renderer/EditorCamera.h>
#include <Heron/Platform/Vulkan/VulkanSwapchain.h>
#include <unordered_map>
#include <memory>

class QLabel;

class TerrainEditorPanel;

class ViewportWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    ViewportWidget(QWidget* parent = nullptr);
    ~ViewportWidget();

    void SetTerrainEditorPanel(TerrainEditorPanel* panel) { m_TerrainEditorPanel = panel; }

    enum class SceneState { Edit, Play };
    void SetSceneState(SceneState state);
    SceneState GetSceneState() const { return m_SceneState; }

    enum class ViewMode { Scene, Game };
    void SetViewMode(ViewMode mode) { m_ViewMode = mode; }
    ViewMode GetViewMode() const { return m_ViewMode; }


    Heron::Scene* GetScene() { return m_ActiveScene.get(); }
    Heron::Ref<Heron::Scene> GetSceneRef() { return m_ActiveScene; }
    void SetScene(Heron::Ref<Heron::Scene> scene);

    enum class GizmoMode { None, Translate, Rotate, Scale };
    void SetGizmoMode(GizmoMode mode) { m_GizmoMode = mode; }
    GizmoMode GetGizmoMode() const { return m_GizmoMode; }

signals:
    void EntitySpawned();
    void SceneChanged(Heron::Scene* newScene);
    void PropertiesChanged();

public slots:
    void SetSelectedEntity(Heron::Entity entity) { m_SelectedEntity = entity; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void updateGameLoop();

    void spawnCube();
    void spawnPlane();
    void spawnCylinder();

private:
    void CreateMeshShader();
    void CreateSkyboxShader();
    void GeneratePrimitiveMeshes();
    void RenderEntities();
    void DrawGizmo();
    void ShowContextMenu(const QPoint& pos);

    // Math Helpers
    void ScreenToWorldRay(int mouseX, int mouseY, glm::vec3& outOrigin, glm::vec3& outDir);
    float RayLineDistance(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& lineOrigin, const glm::vec3& lineDir, float& tLine);
    bool RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& planeOrigin, const glm::vec3& planeNormal, float& t);
    float SnapToIncrement(float value, float increment);

    QTimer m_Timer;
    QPointF m_LastMousePosition;
    bool m_RightClickDragged = false;
    SceneState m_SceneState = SceneState::Edit;
    ViewMode m_ViewMode = ViewMode::Scene;
    GizmoMode m_GizmoMode = GizmoMode::None;
    
    // Gizmo state
    int m_ActiveGizmoAxis = -1; // -1: None, 0: X, 1: Y, 2: Z, 3: Uniform Scale
    bool m_GizmoDragging = false;
    glm::vec3 m_DragStartPoint;
    glm::vec3 m_DragStartEntityPos;
    glm::vec3 m_DragStartEntityRot;
    glm::vec3 m_DragStartEntityScale;

    // Engine Objects
    Heron::Ref<Heron::Scene> m_ActiveScene;
    Heron::Ref<Heron::Scene> m_EditorScene;
    Heron::EditorCamera m_Camera;
    Heron::Entity m_SelectedEntity;
    TerrainEditorPanel* m_TerrainEditorPanel = nullptr;

    // Vulkan Swapchain
    std::shared_ptr<Heron::VulkanSwapchain> m_VulkanSwapchain;
    VkSurfaceKHR m_VulkanSurface = VK_NULL_HANDLE;

    // OpenGL Context
    void* m_hDC = nullptr;
    void* m_hGLRC = nullptr;

    // Mouse Tracking
    QPoint m_LastMousePos;

    // Grid rendering
    Heron::Ref<Heron::Shader> m_GridShader;
    Heron::Ref<Heron::VertexArray> m_GridVAO;

    // Gizmo rendering
    Heron::Ref<Heron::Shader> m_GizmoShader;
    Heron::Ref<Heron::VertexArray> m_GizmoLineVAO;
    Heron::Ref<Heron::VertexArray> m_GizmoCircleVAO;

    // Mesh rendering
    Heron::Ref<Heron::Shader> m_MeshShader;
    std::unordered_map<int, Heron::Ref<Heron::VertexArray>> m_PrimitiveMeshes;
    std::unordered_map<std::string, Heron::Ref<Heron::VertexArray>> m_CustomMeshes;
    Heron::Ref<class Heron::Texture2D> m_WhiteTexture;

    Heron::Ref<Heron::VertexArray> LoadCustomMesh(const std::string& path);

    // Skybox rendering
    Heron::Ref<Heron::Shader> m_SkyboxShader;
    Heron::Ref<Heron::VertexArray> m_SkyboxVAO;

    QLabel* m_ScoreLabel = nullptr;

    int m_SpawnCounter = 0;
    bool m_Initialized = false;
};
