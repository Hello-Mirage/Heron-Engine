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

class PropertiesPanel : public QDockWidget
{
    Q_OBJECT
public:
    PropertiesPanel(QWidget* parent = nullptr);
    ~PropertiesPanel() = default;

    void SetEntity(Heron::Entity entity);

signals:
    void PropertiesChanged();

private slots:
    void OnTagChanged(const QString& text);
    void OnTransformChanged();
    void OnAddRigidBodyComponent();
    void OnAddBoxColliderComponent();
    void OnAddScriptComponent();

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

    // Collider
    QGroupBox* m_BoxColliderGroup;
    QLineEdit* m_SizeX; QLineEdit* m_SizeY; QLineEdit* m_SizeZ;

    QPushButton* m_AddRigidBodyBtn;
    QPushButton* m_AddBoxColliderBtn;
    QPushButton* m_AddScriptBtn;

    // Script
    QGroupBox* m_ScriptGroup;
    QComboBox* m_ScriptCombo;
};
