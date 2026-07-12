#pragma once

#include <QOpenGLWidget>
#include <QTimer>
#include <Heron/Heron.h>
#include <Heron/Renderer/EditorCamera.h>
#include <unordered_map>

class ViewportWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    ViewportWidget(QWidget* parent = nullptr);
    ~ViewportWidget();

    enum class SceneState { Edit, Play };
    void SetSceneState(SceneState state);
    SceneState GetSceneState() const { return m_SceneState; }

    Heron::Scene* GetScene() { return m_ActiveScene.get(); }
    Heron::Ref<Heron::Scene> GetSceneRef() { return m_ActiveScene; }
    void SetScene(Heron::Ref<Heron::Scene> scene);

signals:
    void EntitySpawned();
    void SceneChanged(Heron::Scene* newScene);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void updateGameLoop();

    void spawnCube();
    void spawnPlane();
    void spawnCylinder();

private:
    void CreateMeshShader();
    void GeneratePrimitiveMeshes();
    void RenderEntities();
    void ShowContextMenu(const QPoint& pos);

    QTimer m_Timer;
    QPointF m_LastMousePosition;
    bool m_RightClickDragged = false;
    SceneState m_SceneState = SceneState::Edit;

    // Engine Objects
    Heron::Ref<Heron::Scene> m_ActiveScene;
    Heron::Ref<Heron::Scene> m_EditorScene;
    Heron::EditorCamera m_Camera;

    // Grid rendering
    Heron::Ref<Heron::Shader> m_GridShader;
    Heron::Ref<Heron::VertexArray> m_GridVAO;

    // Mesh rendering
    Heron::Ref<Heron::Shader> m_MeshShader;
    std::unordered_map<int, Heron::Ref<Heron::VertexArray>> m_PrimitiveMeshes;

    int m_SpawnCounter = 0;
};
