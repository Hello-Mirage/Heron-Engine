#include "PropertiesPanel.h"
#include <Heron/Scene/Components.h>
#include <Heron/Scripting/ScriptRegistry.h>
#include <QLabel>
#include <QDoubleValidator>

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QDockWidget("Properties", parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_Container = new QWidget();
    m_MainLayout = new QVBoxLayout(m_Container);
    m_MainLayout->setAlignment(Qt::AlignTop);

    QDoubleValidator* validator = new QDoubleValidator(this);

    // --- Tag Group ---
    m_TagGroup = new QGroupBox("Tag");
    QFormLayout* tagLayout = new QFormLayout(m_TagGroup);
    m_TagEdit = new QLineEdit();
    connect(m_TagEdit, &QLineEdit::textEdited, this, &PropertiesPanel::OnTagChanged);
    tagLayout->addRow("Name", m_TagEdit);
    m_MainLayout->addWidget(m_TagGroup);

    // --- Transform Group ---
    m_TransformGroup = new QGroupBox("Transform");
    QFormLayout* transLayout = new QFormLayout(m_TransformGroup);
    
    m_PosX = new QLineEdit(); m_PosX->setValidator(validator);
    m_PosY = new QLineEdit(); m_PosY->setValidator(validator);
    m_PosZ = new QLineEdit(); m_PosZ->setValidator(validator);
    m_RotX = new QLineEdit(); m_RotX->setValidator(validator);
    m_RotY = new QLineEdit(); m_RotY->setValidator(validator);
    m_RotZ = new QLineEdit(); m_RotZ->setValidator(validator);
    m_ScaleX = new QLineEdit(); m_ScaleX->setValidator(validator);
    m_ScaleY = new QLineEdit(); m_ScaleY->setValidator(validator);
    m_ScaleZ = new QLineEdit(); m_ScaleZ->setValidator(validator);

    auto connectTransform = [this](QLineEdit* edit) {
        connect(edit, &QLineEdit::textEdited, this, &PropertiesPanel::OnTransformChanged);
    };
    connectTransform(m_PosX); connectTransform(m_PosY); connectTransform(m_PosZ);
    connectTransform(m_RotX); connectTransform(m_RotY); connectTransform(m_RotZ);
    connectTransform(m_ScaleX); connectTransform(m_ScaleY); connectTransform(m_ScaleZ);

    transLayout->addRow("Position X", m_PosX);
    transLayout->addRow("Position Y", m_PosY);
    transLayout->addRow("Position Z", m_PosZ);
    transLayout->addRow("Rotation X", m_RotX);
    transLayout->addRow("Rotation Y", m_RotY);
    transLayout->addRow("Rotation Z", m_RotZ);
    transLayout->addRow("Scale X", m_ScaleX);
    transLayout->addRow("Scale Y", m_ScaleY);
    transLayout->addRow("Scale Z", m_ScaleZ);
    m_MainLayout->addWidget(m_TransformGroup);

    // --- RigidBody Group ---
    m_RigidBodyGroup = new QGroupBox("RigidBody");
    QFormLayout* rbLayout = new QFormLayout(m_RigidBodyGroup);
    rbLayout->addRow(new QLabel("Type"), new QLabel("Dynamic Body"));
    m_MainLayout->addWidget(m_RigidBodyGroup);

    // --- Box Collider Group ---
    m_BoxColliderGroup = new QGroupBox("Box Collider");
    QFormLayout* bcLayout = new QFormLayout(m_BoxColliderGroup);
    m_SizeX = new QLineEdit(); m_SizeX->setValidator(validator);
    m_SizeY = new QLineEdit(); m_SizeY->setValidator(validator);
    m_SizeZ = new QLineEdit(); m_SizeZ->setValidator(validator);
    connectTransform(m_SizeX); connectTransform(m_SizeY); connectTransform(m_SizeZ);
    bcLayout->addRow("Size X", m_SizeX);
    bcLayout->addRow("Size Y", m_SizeY);
    bcLayout->addRow("Size Z", m_SizeZ);
    m_MainLayout->addWidget(m_BoxColliderGroup);

    // --- Buttons ---
    m_AddRigidBodyBtn = new QPushButton("Add RigidBody (Physics)");
    connect(m_AddRigidBodyBtn, &QPushButton::clicked, this, &PropertiesPanel::OnAddRigidBodyComponent);
    m_MainLayout->addWidget(m_AddRigidBodyBtn);

    m_AddBoxColliderBtn = new QPushButton("Add Box Collider");
    connect(m_AddBoxColliderBtn, &QPushButton::clicked, this, &PropertiesPanel::OnAddBoxColliderComponent);
    m_MainLayout->addWidget(m_AddBoxColliderBtn);

    // --- Script Group ---
    m_ScriptGroup = new QGroupBox("Script");
    QFormLayout* scriptLayout = new QFormLayout(m_ScriptGroup);
    m_ScriptCombo = new QComboBox();
    scriptLayout->addRow("Class", m_ScriptCombo);
    m_MainLayout->addWidget(m_ScriptGroup);

    // --- Add Script Button ---
    m_AddScriptBtn = new QPushButton("Add Script");
    connect(m_AddScriptBtn, &QPushButton::clicked, this, &PropertiesPanel::OnAddScriptComponent);
    m_MainLayout->addWidget(m_AddScriptBtn);

    setWidget(m_Container);
}

void PropertiesPanel::SetEntity(Heron::Entity entity)
{
    m_SelectionContext = entity;
    DrawComponents();
}

void PropertiesPanel::DrawComponents()
{
    if (!m_SelectionContext) {
        m_TagGroup->hide();
        m_TransformGroup->hide();
        m_RigidBodyGroup->hide();
        m_BoxColliderGroup->hide();
        m_AddRigidBodyBtn->hide();
        m_AddBoxColliderBtn->hide();
        m_ScriptGroup->hide();
        m_AddScriptBtn->hide();
        return;
    }

    if (m_SelectionContext.HasComponent<Heron::TagComponent>()) {
        auto& tag = m_SelectionContext.GetComponent<Heron::TagComponent>().Tag;
        m_TagEdit->setText(QString::fromStdString(tag));
        m_TagGroup->show();
    } else {
        m_TagGroup->hide();
    }

    if (m_SelectionContext.HasComponent<Heron::TransformComponent>()) {
        auto& transform = m_SelectionContext.GetComponent<Heron::TransformComponent>();
        m_PosX->setText(QString::number(transform.Translation[0]));
        m_PosY->setText(QString::number(transform.Translation[1]));
        m_PosZ->setText(QString::number(transform.Translation[2]));
        m_RotX->setText(QString::number(transform.Rotation[0] * 180.0f / 3.14159265f));
        m_RotY->setText(QString::number(transform.Rotation[1] * 180.0f / 3.14159265f));
        m_RotZ->setText(QString::number(transform.Rotation[2] * 180.0f / 3.14159265f));
        m_ScaleX->setText(QString::number(transform.Scale[0]));
        m_ScaleY->setText(QString::number(transform.Scale[1]));
        m_ScaleZ->setText(QString::number(transform.Scale[2]));
        m_TransformGroup->show();
    } else {
        m_TransformGroup->hide();
    }

    if (m_SelectionContext.HasComponent<Heron::RigidBody3DComponent>()) {
        m_RigidBodyGroup->show();
        m_AddRigidBodyBtn->hide();
    } else {
        m_RigidBodyGroup->hide();
        m_AddRigidBodyBtn->show();
    }

    if (m_SelectionContext.HasComponent<Heron::BoxCollider3DComponent>()) {
        auto& collider = m_SelectionContext.GetComponent<Heron::BoxCollider3DComponent>();
        m_SizeX->setText(QString::number(collider.HalfSize[0] * 2.0f));
        m_SizeY->setText(QString::number(collider.HalfSize[1] * 2.0f));
        m_SizeZ->setText(QString::number(collider.HalfSize[2] * 2.0f));
        m_BoxColliderGroup->show();
        m_AddBoxColliderBtn->hide();
    } else {
        m_BoxColliderGroup->hide();
        m_AddBoxColliderBtn->show();
    }

    if (m_SelectionContext.HasComponent<Heron::NativeScriptComponent>()) {
        auto& nsc = m_SelectionContext.GetComponent<Heron::NativeScriptComponent>();
        
        // Populate combo box with registered script names
        m_ScriptCombo->blockSignals(true);
        m_ScriptCombo->clear();
        auto names = Heron::ScriptRegistry::GetRegisteredNames();
        int currentIdx = 0;
        for (int i = 0; i < (int)names.size(); i++) {
            m_ScriptCombo->addItem(QString::fromStdString(names[i]));
            if (names[i] == nsc.ScriptName)
                currentIdx = i;
        }
        m_ScriptCombo->setCurrentIndex(currentIdx);
        m_ScriptCombo->blockSignals(false);

        // Connect combo change to update script name
        disconnect(m_ScriptCombo, nullptr, nullptr, nullptr);
        connect(m_ScriptCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
            if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::NativeScriptComponent>()) {
                m_SelectionContext.GetComponent<Heron::NativeScriptComponent>().ScriptName = text.toStdString();
            }
        });

        m_ScriptGroup->show();
        m_AddScriptBtn->hide();
    } else {
        m_ScriptGroup->hide();
        m_AddScriptBtn->show();
    }
}

void PropertiesPanel::OnTagChanged(const QString& text)
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::TagComponent>()) {
        m_SelectionContext.GetComponent<Heron::TagComponent>().Tag = text.toStdString();
    }
}

void PropertiesPanel::OnTransformChanged()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::TransformComponent>()) {
        auto& transform = m_SelectionContext.GetComponent<Heron::TransformComponent>();
        transform.Translation[0] = m_PosX->text().toFloat();
        transform.Translation[1] = m_PosY->text().toFloat();
        transform.Translation[2] = m_PosZ->text().toFloat();
        
        transform.Rotation[0] = m_RotX->text().toFloat() * 3.14159265f / 180.0f;
        transform.Rotation[1] = m_RotY->text().toFloat() * 3.14159265f / 180.0f;
        transform.Rotation[2] = m_RotZ->text().toFloat() * 3.14159265f / 180.0f;
        
        transform.Scale[0] = m_ScaleX->text().toFloat();
        transform.Scale[1] = m_ScaleY->text().toFloat();
        transform.Scale[2] = m_ScaleZ->text().toFloat();
    }

    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::BoxCollider3DComponent>()) {
        auto& collider = m_SelectionContext.GetComponent<Heron::BoxCollider3DComponent>();
        collider.HalfSize[0] = m_SizeX->text().toFloat() / 2.0f;
        collider.HalfSize[1] = m_SizeY->text().toFloat() / 2.0f;
        collider.HalfSize[2] = m_SizeZ->text().toFloat() / 2.0f;
    }
}

void PropertiesPanel::OnAddRigidBodyComponent()
{
    if (m_SelectionContext && !m_SelectionContext.HasComponent<Heron::RigidBody3DComponent>()) {
        auto& rb = m_SelectionContext.AddComponent<Heron::RigidBody3DComponent>();
        rb.Type = b3_dynamicBody; // Default to falling bodies
        DrawComponents();
    }
}

void PropertiesPanel::OnAddBoxColliderComponent()
{
    if (m_SelectionContext && !m_SelectionContext.HasComponent<Heron::BoxCollider3DComponent>()) {
        auto& collider = m_SelectionContext.AddComponent<Heron::BoxCollider3DComponent>();
        
        // Smart defaults based on visual mesh
        if (m_SelectionContext.HasComponent<Heron::MeshComponent>()) {
            auto& mesh = m_SelectionContext.GetComponent<Heron::MeshComponent>();
            if (mesh.Type == Heron::PrimitiveType::Plane) {
                collider.HalfSize[0] = 5.0f;
                collider.HalfSize[1] = 0.05f;
                collider.HalfSize[2] = 5.0f;
            }
        }
        
        DrawComponents();
    }
}

void PropertiesPanel::OnAddScriptComponent()
{
    if (m_SelectionContext && !m_SelectionContext.HasComponent<Heron::NativeScriptComponent>()) {
        auto& nsc = m_SelectionContext.AddComponent<Heron::NativeScriptComponent>();
        
        // Default to first registered script if available
        auto names = Heron::ScriptRegistry::GetRegisteredNames();
        if (!names.empty())
            nsc.ScriptName = names[0];
        
        DrawComponents();
    }
}
