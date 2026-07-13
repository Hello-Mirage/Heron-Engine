#pragma once

#include <QDockWidget>
#include <Heron/Scene/Scene.h>
#include <Heron/Scene/Entity.h>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>

class PropertiesPanel : public QDockWidget
{
    Q_OBJECT
public:
    PropertiesPanel(QWidget* parent = nullptr);
    ~PropertiesPanel() = default;

    void SetEntity(Heron::Entity entity);
    void UpdateValues();

signals:
    void PropertiesChanged();

private slots:
    void OnTagChanged(const QString& text);
    void OnTransformChanged();
    void OnFixedRotationChanged();
    void OnAddRigidBodyComponent();
    void OnAddBoxColliderComponent();
    void OnAddScriptComponent();
    void OnAddCameraComponent();
    void OnCameraChanged();
    void OnMeshColorClicked();
    void OnMeshTextureClicked();
    void OnAddLightComponent();
    void OnLightColorClicked();
    void OnLightIntensityChanged();
private:
    void DrawComponents();

    Heron::Entity m_SelectionContext;
    QWidget* m_Container;
    QVBoxLayout* m_MainLayout;

    // Tag
    QGroupBox* m_TagGroup;
    QLineEdit* m_TagEdit;

    // Transform
    QGroupBox* m_TransformGroup;
    QLineEdit* m_PosX; QLineEdit* m_PosY; QLineEdit* m_PosZ;
    QLineEdit* m_RotX; QLineEdit* m_RotY; QLineEdit* m_RotZ;
    QLineEdit* m_ScaleX; QLineEdit* m_ScaleY; QLineEdit* m_ScaleZ;

    // RigidBody
    QGroupBox* m_RigidBodyGroup;
    QCheckBox* m_FixedRotationCB;

    // Collider
    QGroupBox* m_BoxColliderGroup;
    QLineEdit* m_SizeX; QLineEdit* m_SizeY; QLineEdit* m_SizeZ;

    // Camera
    QGroupBox* m_CameraGroup;
    QCheckBox* m_CameraPrimary;
    QLineEdit* m_CameraFOV; QLineEdit* m_CameraNear; QLineEdit* m_CameraFar;

    // Mesh
    QGroupBox* m_MeshGroup;
    QPushButton* m_MeshColorBtn;
    QPushButton* m_MeshTextureBtn;
    QLabel* m_MeshTextureLabel;

    QPushButton* m_AddRigidBodyBtn;
    QPushButton* m_AddBoxColliderBtn;
    QPushButton* m_AddScriptBtn;
    QPushButton* m_AddCameraBtn;

    // Script
    QGroupBox* m_ScriptGroup;
    QComboBox* m_ScriptCombo;

    // Light
    QGroupBox* m_LightGroup;
    QPushButton* m_LightColorBtn;
    QLineEdit* m_LightIntensity;
    QPushButton* m_AddLightBtn;

    // Skybox
    QGroupBox* m_SkyboxGroup;
    QPushButton* m_AddSkyboxBtn;
    QPushButton* m_SkyboxBtnRight;
    QPushButton* m_SkyboxBtnLeft;
    QPushButton* m_SkyboxBtnTop;
    QPushButton* m_SkyboxBtnBottom;
    QPushButton* m_SkyboxBtnFront;
    QPushButton* m_SkyboxBtnBack;

private slots:
    void OnAddSkyboxComponent();
    void OnSkyboxTextureClicked(int faceIndex);

};
