#include "PropertiesPanel.h"
#include <Heron/Scene/Components.h>
#include <Heron/Scripting/ScriptRegistry.h>
#include <QLabel>
#include <QDoubleValidator>
#include <QColorDialog>
#include <QFileDialog>
#include <Heron/Renderer/Texture.h>
#include <QScrollArea>

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QDockWidget("Properties", parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

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
    m_FixedRotationCB = new QCheckBox();
    connect(m_FixedRotationCB, &QCheckBox::stateChanged, this, &PropertiesPanel::OnFixedRotationChanged);
    rbLayout->addRow("Fixed Rotation", m_FixedRotationCB);
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

    // --- Camera Group ---
    m_CameraGroup = new QGroupBox("Camera");
    QFormLayout* cameraLayout = new QFormLayout(m_CameraGroup);
    
    m_CameraPrimary = new QCheckBox();
    m_CameraFOV = new QLineEdit(); m_CameraFOV->setValidator(validator);
    m_CameraNear = new QLineEdit(); m_CameraNear->setValidator(validator);
    m_CameraFar = new QLineEdit(); m_CameraFar->setValidator(validator);

    connect(m_CameraPrimary, &QCheckBox::stateChanged, this, &PropertiesPanel::OnCameraChanged);
    connect(m_CameraFOV, &QLineEdit::textEdited, this, &PropertiesPanel::OnCameraChanged);
    connect(m_CameraNear, &QLineEdit::textEdited, this, &PropertiesPanel::OnCameraChanged);
    connect(m_CameraFar, &QLineEdit::textEdited, this, &PropertiesPanel::OnCameraChanged);

    cameraLayout->addRow("Primary", m_CameraPrimary);
    cameraLayout->addRow("FOV", m_CameraFOV);
    cameraLayout->addRow("Near Clip", m_CameraNear);
    cameraLayout->addRow("Far Clip", m_CameraFar);
    m_MainLayout->addWidget(m_CameraGroup);

    // --- Mesh Group ---
    m_MeshGroup = new QGroupBox("Mesh Material");
    QFormLayout* meshLayout = new QFormLayout(m_MeshGroup);
    m_MeshColorBtn = new QPushButton("Pick Color");
    m_MeshTextureBtn = new QPushButton("Select Texture...");
    m_MeshTextureLabel = new QLabel("None");

    connect(m_MeshColorBtn, &QPushButton::clicked, this, &PropertiesPanel::OnMeshColorClicked);
    connect(m_MeshTextureBtn, &QPushButton::clicked, this, &PropertiesPanel::OnMeshTextureClicked);

    meshLayout->addRow("Color", m_MeshColorBtn);
    meshLayout->addRow("Texture", m_MeshTextureBtn);
    meshLayout->addRow("Current:", m_MeshTextureLabel);
    m_MainLayout->addWidget(m_MeshGroup);

    // --- Add Camera Button ---
    m_AddCameraBtn = new QPushButton("Add Camera");
    connect(m_AddCameraBtn, &QPushButton::clicked, this, &PropertiesPanel::OnAddCameraComponent);
    m_MainLayout->addWidget(m_AddCameraBtn);

    // --- Light Group ---
    m_LightGroup = new QGroupBox("Light");
    QFormLayout* lightLayout = new QFormLayout(m_LightGroup);
    m_LightColorBtn = new QPushButton("Pick Color");
    m_LightIntensity = new QLineEdit(); m_LightIntensity->setValidator(validator);
    
    connect(m_LightColorBtn, &QPushButton::clicked, this, &PropertiesPanel::OnLightColorClicked);
    connect(m_LightIntensity, &QLineEdit::textEdited, this, &PropertiesPanel::OnLightIntensityChanged);
    
    lightLayout->addRow("Color", m_LightColorBtn);
    lightLayout->addRow("Intensity", m_LightIntensity);
    m_MainLayout->addWidget(m_LightGroup);

    m_AddLightBtn = new QPushButton("Add Light");
    connect(m_AddLightBtn, &QPushButton::clicked, this, &PropertiesPanel::OnAddLightComponent);
    m_MainLayout->addWidget(m_AddLightBtn);

    // --- Skybox Group ---
    m_SkyboxGroup = new QGroupBox("Skybox");
    QFormLayout* skyboxLayout = new QFormLayout(m_SkyboxGroup);
    m_SkyboxBtnRight = new QPushButton("Select Right...");
    m_SkyboxBtnLeft = new QPushButton("Select Left...");
    m_SkyboxBtnTop = new QPushButton("Select Top...");
    m_SkyboxBtnBottom = new QPushButton("Select Bottom...");
    m_SkyboxBtnFront = new QPushButton("Select Front...");
    m_SkyboxBtnBack = new QPushButton("Select Back...");

    connect(m_SkyboxBtnRight, &QPushButton::clicked, this, [this]() { OnSkyboxTextureClicked(0); });
    connect(m_SkyboxBtnLeft, &QPushButton::clicked, this, [this]() { OnSkyboxTextureClicked(1); });
    connect(m_SkyboxBtnTop, &QPushButton::clicked, this, [this]() { OnSkyboxTextureClicked(2); });
    connect(m_SkyboxBtnBottom, &QPushButton::clicked, this, [this]() { OnSkyboxTextureClicked(3); });
    connect(m_SkyboxBtnFront, &QPushButton::clicked, this, [this]() { OnSkyboxTextureClicked(4); });
    connect(m_SkyboxBtnBack, &QPushButton::clicked, this, [this]() { OnSkyboxTextureClicked(5); });

    skyboxLayout->addRow("Right (+X)", m_SkyboxBtnRight);
    skyboxLayout->addRow("Left (-X)", m_SkyboxBtnLeft);
    skyboxLayout->addRow("Top (+Y)", m_SkyboxBtnTop);
    skyboxLayout->addRow("Bottom (-Y)", m_SkyboxBtnBottom);
    skyboxLayout->addRow("Front (+Z)", m_SkyboxBtnFront);
    skyboxLayout->addRow("Back (-Z)", m_SkyboxBtnBack);
    m_MainLayout->addWidget(m_SkyboxGroup);

    m_AddSkyboxBtn = new QPushButton("Add Skybox");
    connect(m_AddSkyboxBtn, &QPushButton::clicked, this, &PropertiesPanel::OnAddSkyboxComponent);
    m_MainLayout->addWidget(m_AddSkyboxBtn);

    scrollArea->setWidget(m_Container);
    setWidget(scrollArea);
}

void PropertiesPanel::SetEntity(Heron::Entity entity)
{
    m_SelectionContext = entity;
    DrawComponents();
}

void PropertiesPanel::UpdateValues()
{
    if (!m_SelectionContext) return;

    if (m_SelectionContext.HasComponent<Heron::TransformComponent>()) {
        auto& transform = m_SelectionContext.GetComponent<Heron::TransformComponent>();
        
        // Block signals to prevent recursive updates
        m_PosX->blockSignals(true); m_PosY->blockSignals(true); m_PosZ->blockSignals(true);
        m_RotX->blockSignals(true); m_RotY->blockSignals(true); m_RotZ->blockSignals(true);
        m_ScaleX->blockSignals(true); m_ScaleY->blockSignals(true); m_ScaleZ->blockSignals(true);

        m_PosX->setText(QString::number(transform.Translation[0]));
        m_PosY->setText(QString::number(transform.Translation[1]));
        m_PosZ->setText(QString::number(transform.Translation[2]));
        m_RotX->setText(QString::number(glm::degrees(transform.Rotation[0])));
        m_RotY->setText(QString::number(glm::degrees(transform.Rotation[1])));
        m_RotZ->setText(QString::number(glm::degrees(transform.Rotation[2])));
        m_ScaleX->setText(QString::number(transform.Scale[0]));
        m_ScaleY->setText(QString::number(transform.Scale[1]));
        m_ScaleZ->setText(QString::number(transform.Scale[2]));

        m_PosX->blockSignals(false); m_PosY->blockSignals(false); m_PosZ->blockSignals(false);
        m_RotX->blockSignals(false); m_RotY->blockSignals(false); m_RotZ->blockSignals(false);
        m_ScaleX->blockSignals(false); m_ScaleY->blockSignals(false); m_ScaleZ->blockSignals(false);
    }
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
        m_CameraGroup->hide();
        m_AddCameraBtn->hide();
        m_MeshGroup->hide();
        m_LightGroup->hide();
        m_AddLightBtn->hide();
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
        auto& rb = m_SelectionContext.GetComponent<Heron::RigidBody3DComponent>();
        m_FixedRotationCB->blockSignals(true);
        m_FixedRotationCB->setChecked(rb.FixedRotation);
        m_FixedRotationCB->blockSignals(false);
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

    if (m_SelectionContext.HasComponent<Heron::CameraComponent>()) {
        auto& camera = m_SelectionContext.GetComponent<Heron::CameraComponent>();
        m_CameraPrimary->blockSignals(true);
        m_CameraFOV->blockSignals(true);
        m_CameraNear->blockSignals(true);
        m_CameraFar->blockSignals(true);
        
        m_CameraPrimary->setChecked(camera.Primary);
        m_CameraFOV->setText(QString::number(camera.FOV));
        m_CameraNear->setText(QString::number(camera.NearClip));
        m_CameraFar->setText(QString::number(camera.FarClip));
        
        m_CameraPrimary->blockSignals(false);
        m_CameraFOV->blockSignals(false);
        m_CameraNear->blockSignals(false);
        m_CameraFar->blockSignals(false);

        m_CameraGroup->show();
        m_AddCameraBtn->hide();
    } else {
        m_CameraGroup->hide();
        m_AddCameraBtn->show();
    }

    if (m_SelectionContext.HasComponent<Heron::MeshComponent>()) {
        auto& mesh = m_SelectionContext.GetComponent<Heron::MeshComponent>();
        
        QColor c((int)(mesh.Color[0]*255), (int)(mesh.Color[1]*255), (int)(mesh.Color[2]*255), (int)(mesh.Color[3]*255));
        QString style = QString("background-color: %1;").arg(c.name());
        m_MeshColorBtn->setStyleSheet(style);

        if (mesh.TexturePath.empty()) {
            m_MeshTextureLabel->setText("None");
        } else {
            QString path = QString::fromStdString(mesh.TexturePath);
            QFileInfo fi(path);
            m_MeshTextureLabel->setText(fi.fileName());
        }
        
        m_MeshGroup->show();
    } else {
        m_MeshGroup->hide();
    }

    if (m_SelectionContext.HasComponent<Heron::LightComponent>()) {
        auto& light = m_SelectionContext.GetComponent<Heron::LightComponent>();
        
        QColor c((int)(light.Color[0]*255), (int)(light.Color[1]*255), (int)(light.Color[2]*255));
        QString style = QString("background-color: %1;").arg(c.name());
        m_LightColorBtn->setStyleSheet(style);

        m_LightIntensity->blockSignals(true);
        m_LightIntensity->setText(QString::number(light.Intensity));
        m_LightIntensity->blockSignals(false);

        m_LightGroup->show();
        m_AddLightBtn->hide();
    } else {
        m_LightGroup->hide();
        m_AddLightBtn->show();
    }

    if (m_SelectionContext.HasComponent<Heron::SkyboxComponent>()) {
        auto& sc = m_SelectionContext.GetComponent<Heron::SkyboxComponent>();
        
        auto setBtnText = [](QPushButton* btn, const std::string& path, const char* defaultText) {
            if (path.empty()) btn->setText(defaultText);
            else {
                QFileInfo fi(QString::fromStdString(path));
                btn->setText(fi.fileName());
            }
        };

        setBtnText(m_SkyboxBtnRight, sc.TexturePaths[0], "Select Right...");
        setBtnText(m_SkyboxBtnLeft, sc.TexturePaths[1], "Select Left...");
        setBtnText(m_SkyboxBtnTop, sc.TexturePaths[2], "Select Top...");
        setBtnText(m_SkyboxBtnBottom, sc.TexturePaths[3], "Select Bottom...");
        setBtnText(m_SkyboxBtnFront, sc.TexturePaths[4], "Select Front...");
        setBtnText(m_SkyboxBtnBack, sc.TexturePaths[5], "Select Back...");

        m_SkyboxGroup->show();
        m_AddSkyboxBtn->hide();
    } else {
        m_SkyboxGroup->hide();
        m_AddSkyboxBtn->show();
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

void PropertiesPanel::OnFixedRotationChanged()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::RigidBody3DComponent>()) {
        auto& rb = m_SelectionContext.GetComponent<Heron::RigidBody3DComponent>();
        rb.FixedRotation = m_FixedRotationCB->isChecked();
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

void PropertiesPanel::OnAddCameraComponent()
{
    if (m_SelectionContext && !m_SelectionContext.HasComponent<Heron::CameraComponent>()) {
        m_SelectionContext.AddComponent<Heron::CameraComponent>();
        DrawComponents();
    }
}

void PropertiesPanel::OnCameraChanged()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::CameraComponent>()) {
        auto& camera = m_SelectionContext.GetComponent<Heron::CameraComponent>();
        camera.Primary = m_CameraPrimary->isChecked();
        camera.FOV = m_CameraFOV->text().toFloat();
        camera.NearClip = m_CameraNear->text().toFloat();
        camera.FarClip = m_CameraFar->text().toFloat();
    }
}

void PropertiesPanel::OnMeshColorClicked()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::MeshComponent>()) {
        auto& mesh = m_SelectionContext.GetComponent<Heron::MeshComponent>();
        QColor initial((int)(mesh.Color[0]*255), (int)(mesh.Color[1]*255), (int)(mesh.Color[2]*255), (int)(mesh.Color[3]*255));
        
        QColor newColor = QColorDialog::getColor(initial, this, "Pick Mesh Color", QColorDialog::ShowAlphaChannel);
        if (newColor.isValid()) {
            mesh.Color[0] = newColor.redF();
            mesh.Color[1] = newColor.greenF();
            mesh.Color[2] = newColor.blueF();
            mesh.Color[3] = newColor.alphaF();
            DrawComponents();
        }
    }
}

void PropertiesPanel::OnMeshTextureClicked()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::MeshComponent>()) {
        QString fileName = QFileDialog::getOpenFileName(this, "Select Texture", "", "Images (*.png *.jpg *.jpeg *.bmp *.tga)");
        if (!fileName.isEmpty()) {
            auto& mesh = m_SelectionContext.GetComponent<Heron::MeshComponent>();
            mesh.TexturePath = fileName.toStdString();
            mesh.Texture = Heron::Texture2D::Create(mesh.TexturePath);
            DrawComponents();
        }
    }
}

void PropertiesPanel::OnAddLightComponent()
{
    if (m_SelectionContext && !m_SelectionContext.HasComponent<Heron::LightComponent>()) {
        m_SelectionContext.AddComponent<Heron::LightComponent>();
        DrawComponents();
    }
}

void PropertiesPanel::OnLightColorClicked()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::LightComponent>()) {
        auto& light = m_SelectionContext.GetComponent<Heron::LightComponent>();
        QColor initial((int)(light.Color[0]*255), (int)(light.Color[1]*255), (int)(light.Color[2]*255));
        
        QColor newColor = QColorDialog::getColor(initial, this, "Pick Light Color");
        if (newColor.isValid()) {
            light.Color[0] = newColor.redF();
            light.Color[1] = newColor.greenF();
            light.Color[2] = newColor.blueF();
            DrawComponents();
        }
    }
}

void PropertiesPanel::OnLightIntensityChanged()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::LightComponent>()) {
        auto& light = m_SelectionContext.GetComponent<Heron::LightComponent>();
        light.Intensity = m_LightIntensity->text().toFloat();
    }
}

void PropertiesPanel::OnAddSkyboxComponent()
{
    if (m_SelectionContext && !m_SelectionContext.HasComponent<Heron::SkyboxComponent>()) {
        m_SelectionContext.AddComponent<Heron::SkyboxComponent>();
        DrawComponents();
    }
}

void PropertiesPanel::OnSkyboxTextureClicked(int faceIndex)
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::SkyboxComponent>()) {
        QString fileName = QFileDialog::getOpenFileName(this, "Select Skybox Texture", "", "Images (*.png *.jpg *.jpeg *.bmp *.tga)");
        if (!fileName.isEmpty()) {
            auto& sc = m_SelectionContext.GetComponent<Heron::SkyboxComponent>();
            sc.TexturePaths[faceIndex] = fileName.toStdString();
            
            // Try to load cubemap if all faces are present
            bool hasAll = true;
            for (int i=0; i<6; i++) {
                if (sc.TexturePaths[i].empty()) hasAll = false;
            }
            if (hasAll) {
                std::vector<std::string> paths;
                for (int i=0; i<6; i++) paths.push_back(sc.TexturePaths[i]);
                sc.Texture = Heron::TextureCube::Create(paths);
            }
            
            DrawComponents();
        }
    }
}
