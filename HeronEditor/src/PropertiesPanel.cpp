#include "PropertiesPanel.h"
#include <Heron/Scene/Components.h>
#include <Heron/Scripting/ScriptRegistry.h>
#include <QLabel>
#include <QDoubleValidator>
#include <QColorDialog>
#include <QFileDialog>
#include <Heron/Renderer/Texture.h>
#include <QScrollArea>
#include <QIntValidator>

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

    cameraLayout->addRow("Main Camera", m_CameraPrimary);
    cameraLayout->addRow("FOV", m_CameraFOV);
    cameraLayout->addRow("Near Clip", m_CameraNear);
    cameraLayout->addRow("Far Clip", m_CameraFar);
    m_MainLayout->addWidget(m_CameraGroup);

    // --- Mesh Group ---
    m_MeshGroup = new QGroupBox("Mesh Material");
    QFormLayout* meshLayout = new QFormLayout(m_MeshGroup);
    m_MeshTypeCombo = new QComboBox();
    m_MeshTypeCombo->addItems({"Cube", "Plane", "Cylinder", "Sphere", "Custom"});
    m_MeshColorBtn = new QPushButton("Pick Color");
    m_MeshTextureBtn = new QPushButton("Select Texture...");
    m_MeshTextureLabel = new QLabel("None");
    m_MeshModelBtn = new QPushButton("Select Model (.obj)...");
    m_MeshModelLabel = new QLabel("None");

    connect(m_MeshTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PropertiesPanel::OnMeshTypeChanged);
    connect(m_MeshColorBtn, &QPushButton::clicked, this, &PropertiesPanel::OnMeshColorClicked);
    connect(m_MeshTextureBtn, &QPushButton::clicked, this, &PropertiesPanel::OnMeshTextureClicked);
    connect(m_MeshModelBtn, &QPushButton::clicked, this, &PropertiesPanel::OnMeshModelClicked);

    meshLayout->addRow("Type", m_MeshTypeCombo);
    meshLayout->addRow("Model File", m_MeshModelBtn);
    meshLayout->addRow("Current Model:", m_MeshModelLabel);
    meshLayout->addRow("Color", m_MeshColorBtn);
    meshLayout->addRow("Texture", m_MeshTextureBtn);
    meshLayout->addRow("Current Tex:", m_MeshTextureLabel);
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

    // --- Audio Group ---
    m_AudioGroup = new QGroupBox("Audio Player");
    QVBoxLayout* audioGroupLayout = new QVBoxLayout(m_AudioGroup);

    // Checkboxes row
    QHBoxLayout* audioCtrlRow = new QHBoxLayout();
    m_AudioPlayCB   = new QCheckBox("Play");
    m_AudioShuffleCB= new QCheckBox("Shuffle");
    m_AudioLoopCB   = new QCheckBox("Loop");
    audioCtrlRow->addWidget(m_AudioPlayCB);
    audioCtrlRow->addWidget(m_AudioShuffleCB);
    audioCtrlRow->addWidget(m_AudioLoopCB);
    audioGroupLayout->addLayout(audioCtrlRow);

    connect(m_AudioPlayCB,    &QCheckBox::stateChanged, this, &PropertiesPanel::OnAudioPlayChanged);
    connect(m_AudioShuffleCB, &QCheckBox::stateChanged, this, &PropertiesPanel::OnAudioShuffleChanged);
    connect(m_AudioLoopCB,    &QCheckBox::stateChanged, this, &PropertiesPanel::OnAudioLoopChanged);

    // Track list container (dynamically rebuilt)
    m_AudioTrackListWidget = new QWidget();
    m_AudioTrackListLayout = new QVBoxLayout(m_AudioTrackListWidget);
    m_AudioTrackListLayout->setContentsMargins(0, 0, 0, 0);
    m_AudioTrackListLayout->setSpacing(2);
    audioGroupLayout->addWidget(m_AudioTrackListWidget);

    // Add Track button
    m_AudioAddTrackBtn = new QPushButton("+ Add Track");
    connect(m_AudioAddTrackBtn, &QPushButton::clicked, this, &PropertiesPanel::OnAudioAddTrack);
    audioGroupLayout->addWidget(m_AudioAddTrackBtn);

    m_MainLayout->addWidget(m_AudioGroup);

    m_AddAudioBtn = new QPushButton("Add Audio Player");
    connect(m_AddAudioBtn, &QPushButton::clicked, this, &PropertiesPanel::OnAddAudioComponent);
    m_MainLayout->addWidget(m_AddAudioBtn);

    // --- Terrain Group ---
    m_TerrainGroup = new QGroupBox("Terrain");
    QFormLayout* terrainLayout = new QFormLayout(m_TerrainGroup);
    m_TerrainHeightmapBtn = new QPushButton("Select Heightmap...");
    m_TerrainHeightmapLabel = new QLabel("None");
    m_TerrainTextureBtn = new QPushButton("Select Texture...");
    m_TerrainTextureLabel = new QLabel("None");
    
    m_TerrainSize = new QLineEdit(); m_TerrainSize->setValidator(validator);
    m_TerrainHeightScale = new QLineEdit(); m_TerrainHeightScale->setValidator(validator);
    m_TerrainResolution = new QLineEdit(); m_TerrainResolution->setValidator(new QIntValidator(this));

    connect(m_TerrainHeightmapBtn, &QPushButton::clicked, this, &PropertiesPanel::OnTerrainHeightmapClicked);
    connect(m_TerrainTextureBtn, &QPushButton::clicked, this, &PropertiesPanel::OnTerrainTextureClicked);
    connect(m_TerrainSize, &QLineEdit::textEdited, this, &PropertiesPanel::OnTerrainValuesChanged);
    connect(m_TerrainHeightScale, &QLineEdit::textEdited, this, &PropertiesPanel::OnTerrainValuesChanged);
    connect(m_TerrainResolution, &QLineEdit::textEdited, this, &PropertiesPanel::OnTerrainValuesChanged);

    terrainLayout->addRow("Heightmap", m_TerrainHeightmapBtn);
    terrainLayout->addRow("Current Map:", m_TerrainHeightmapLabel);
    terrainLayout->addRow("Texture", m_TerrainTextureBtn);
    terrainLayout->addRow("Current Tex:", m_TerrainTextureLabel);
    terrainLayout->addRow("Size (Width/Depth)", m_TerrainSize);
    terrainLayout->addRow("Height Scale", m_TerrainHeightScale);
    terrainLayout->addRow("Resolution", m_TerrainResolution);
    m_MainLayout->addWidget(m_TerrainGroup);

    m_AddTerrainBtn = new QPushButton("Add Terrain");
    connect(m_AddTerrainBtn, &QPushButton::clicked, this, &PropertiesPanel::OnAddTerrainComponent);
    m_MainLayout->addWidget(m_AddTerrainBtn);

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
        m_AudioGroup->hide();
        m_AddAudioBtn->hide();
        m_TerrainGroup->hide();
        m_AddTerrainBtn->hide();
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
        
        m_MeshTypeCombo->blockSignals(true);
        if (mesh.Type == Heron::PrimitiveType::Cube) m_MeshTypeCombo->setCurrentIndex(0);
        else if (mesh.Type == Heron::PrimitiveType::Plane) m_MeshTypeCombo->setCurrentIndex(1);
        else if (mesh.Type == Heron::PrimitiveType::Cylinder) m_MeshTypeCombo->setCurrentIndex(2);
        else if (mesh.Type == Heron::PrimitiveType::Sphere) m_MeshTypeCombo->setCurrentIndex(3);
        else if (mesh.Type == Heron::PrimitiveType::Custom) m_MeshTypeCombo->setCurrentIndex(4);
        m_MeshTypeCombo->blockSignals(false);

        if (mesh.Type == Heron::PrimitiveType::Custom) {
            m_MeshModelBtn->show();
            m_MeshModelLabel->show();
            if (mesh.ModelPath.empty()) {
                m_MeshModelLabel->setText("None");
            } else {
                QFileInfo fi(QString::fromStdString(mesh.ModelPath));
                m_MeshModelLabel->setText(fi.fileName());
            }
        } else {
            m_MeshModelBtn->hide();
            m_MeshModelLabel->hide();
        }

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
        HRN_CORE_INFO("DrawComponents: Has SkyboxComponent");
        auto& sc = m_SelectionContext.GetComponent<Heron::SkyboxComponent>();
        
        auto setBtnText = [](QPushButton* btn, const std::string& path, const char* defaultText) {
            HRN_CORE_INFO("setBtnText start: path={0}", path);
            if (!btn) {
                HRN_CORE_ERROR("btn is null!");
                return;
            }
            if (path.empty()) {
                HRN_CORE_INFO("path empty, setting default text");
                btn->setText(defaultText);
            } else {
                HRN_CORE_INFO("path not empty, creating QFileInfo");
                QFileInfo fi(QString::fromStdString(path));
                HRN_CORE_INFO("getting fileName");
                QString fileName = fi.fileName();
                HRN_CORE_INFO("setting text");
                btn->setText(fileName);
            }
            HRN_CORE_INFO("setBtnText done");
        };

        HRN_CORE_INFO("DrawComponents: Setting button texts");
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

    // --- Audio Component ---
    if (m_SelectionContext.HasComponent<Heron::AudioComponent>()) {
        auto& ac = m_SelectionContext.GetComponent<Heron::AudioComponent>();

        m_AudioPlayCB->blockSignals(true);
        m_AudioShuffleCB->blockSignals(true);
        m_AudioLoopCB->blockSignals(true);
        m_AudioPlayCB->setChecked(ac.Play);
        m_AudioShuffleCB->setChecked(ac.Shuffle);
        m_AudioLoopCB->setChecked(ac.Loop);
        m_AudioPlayCB->blockSignals(false);
        m_AudioShuffleCB->blockSignals(false);
        m_AudioLoopCB->blockSignals(false);

        // Rebuild the dynamic track list
        QLayoutItem* child;
        while ((child = m_AudioTrackListLayout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
        for (int i = 0; i < (int)ac.AudioPaths.size(); i++) {
            QWidget* row = new QWidget();
            QHBoxLayout* rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);

            QFileInfo fi(QString::fromStdString(ac.AudioPaths[i]));
            QLabel* label = new QLabel(fi.fileName().isEmpty() ? QString::fromStdString(ac.AudioPaths[i]) : fi.fileName());
            label->setToolTip(QString::fromStdString(ac.AudioPaths[i]));

            QPushButton* removeBtn = new QPushButton("X");
            removeBtn->setFixedWidth(24);
            int idx = i;
            connect(removeBtn, &QPushButton::clicked, this, [this, idx]() { OnAudioRemoveTrack(idx); });

            rowLayout->addWidget(label, 1);
            rowLayout->addWidget(removeBtn);
            m_AudioTrackListLayout->addWidget(row);
        }

        // Show currently playing track if any
        if (ac.CurrentTrackIndex >= 0 && ac.CurrentTrackIndex < (int)ac.AudioPaths.size()) {
            QFileInfo fi(QString::fromStdString(ac.AudioPaths[ac.CurrentTrackIndex]));
            m_AudioGroup->setTitle(QString("Audio Player ▶ %1").arg(fi.fileName()));
        } else {
            m_AudioGroup->setTitle("Audio Player");
        }

        m_AudioGroup->show();
        m_AddAudioBtn->hide();
    } else {
        m_AudioGroup->hide();
        m_AddAudioBtn->show();
    }
    // --- Terrain Component ---
    if (m_SelectionContext.HasComponent<Heron::TerrainComponent>()) {
        auto& terrain = m_SelectionContext.GetComponent<Heron::TerrainComponent>();
        
        m_TerrainSize->blockSignals(true);
        m_TerrainHeightScale->blockSignals(true);
        m_TerrainResolution->blockSignals(true);
        
        m_TerrainSize->setText(QString::number(terrain.Size));
        m_TerrainHeightScale->setText(QString::number(terrain.HeightScale));
        m_TerrainResolution->setText(QString::number(terrain.Resolution));
        
        m_TerrainSize->blockSignals(false);
        m_TerrainHeightScale->blockSignals(false);
        m_TerrainResolution->blockSignals(false);

        if (terrain.HeightmapPath.empty()) {
            m_TerrainHeightmapLabel->setText("None");
        } else {
            QFileInfo fi(QString::fromStdString(terrain.HeightmapPath));
            m_TerrainHeightmapLabel->setText(fi.fileName());
        }

        if (terrain.TexturePath.empty()) {
            m_TerrainTextureLabel->setText("None");
        } else {
            QFileInfo fi(QString::fromStdString(terrain.TexturePath));
            m_TerrainTextureLabel->setText(fi.fileName());
        }

        m_TerrainGroup->show();
        m_AddTerrainBtn->hide();
    } else {
        m_TerrainGroup->hide();
        m_AddTerrainBtn->show();
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

void PropertiesPanel::OnMeshTypeChanged(int index)
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::MeshComponent>()) {
        auto& mesh = m_SelectionContext.GetComponent<Heron::MeshComponent>();
        if (index == 0) mesh.Type = Heron::PrimitiveType::Cube;
        else if (index == 1) mesh.Type = Heron::PrimitiveType::Plane;
        else if (index == 2) mesh.Type = Heron::PrimitiveType::Cylinder;
        else if (index == 3) mesh.Type = Heron::PrimitiveType::Sphere;
        else if (index == 4) mesh.Type = Heron::PrimitiveType::Custom;
        DrawComponents();
    }
}

void PropertiesPanel::OnMeshModelClicked()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::MeshComponent>()) {
        QString fileName = QFileDialog::getOpenFileName(this, "Select Model", "", "OBJ Files (*.obj)");
        if (!fileName.isEmpty()) {
            auto& mesh = m_SelectionContext.GetComponent<Heron::MeshComponent>();
            mesh.ModelPath = fileName.toStdString();
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
    HRN_CORE_INFO("PropertiesPanel::OnAddSkyboxComponent triggered");
    if (m_SelectionContext && !m_SelectionContext.HasComponent<Heron::SkyboxComponent>()) {
        HRN_CORE_INFO("Adding SkyboxComponent");
        auto& sc = m_SelectionContext.AddComponent<Heron::SkyboxComponent>();
        
        // Set default skybox images using absolute paths to avoid working directory issues
        sc.TexturePaths[0] = "D:/Heron-Engine/Skyboxdefault/r.png";
        sc.TexturePaths[1] = "D:/Heron-Engine/Skyboxdefault/L.png";
        sc.TexturePaths[2] = "D:/Heron-Engine/Skyboxdefault/t.png";
        sc.TexturePaths[3] = "D:/Heron-Engine/Skyboxdefault/bottom.png";
        sc.TexturePaths[4] = "D:/Heron-Engine/Skyboxdefault/f.png";
        sc.TexturePaths[5] = "D:/Heron-Engine/Skyboxdefault/b.png";
        sc.Texture = nullptr;
        
        HRN_CORE_INFO("Calling DrawComponents from OnAddSkyboxComponent");
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
            sc.Texture = nullptr;
            
            // The actual OpenGL texture creation is handled in ViewportWidget
            // to ensure the OpenGL context is current.
            
            DrawComponents();
        }
    }
}

// ===================== Audio Slots =====================

void PropertiesPanel::OnAddAudioComponent()
{
    if (m_SelectionContext && !m_SelectionContext.HasComponent<Heron::AudioComponent>()) {
        m_SelectionContext.AddComponent<Heron::AudioComponent>();
        DrawComponents();
    }
}

void PropertiesPanel::OnAudioAddTrack()
{
    if (!m_SelectionContext || !m_SelectionContext.HasComponent<Heron::AudioComponent>()) return;

    QString fileName = QFileDialog::getOpenFileName(
        this, "Select Audio Track", "",
        "Audio Files (*.mp3 *.wav *.flac *.ogg *.opus *.aiff)");
    if (!fileName.isEmpty()) {
        auto& ac = m_SelectionContext.GetComponent<Heron::AudioComponent>();
        ac.AudioPaths.push_back(fileName.toStdString());
        DrawComponents();
    }
}

void PropertiesPanel::OnAudioRemoveTrack(int index)
{
    if (!m_SelectionContext || !m_SelectionContext.HasComponent<Heron::AudioComponent>()) return;
    auto& ac = m_SelectionContext.GetComponent<Heron::AudioComponent>();
    if (index >= 0 && index < (int)ac.AudioPaths.size()) {
        ac.AudioPaths.erase(ac.AudioPaths.begin() + index);
        if (ac.CurrentTrackIndex >= (int)ac.AudioPaths.size())
            ac.CurrentTrackIndex = -1;
        DrawComponents();
    }
}

void PropertiesPanel::OnAudioPlayChanged(int state)
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::AudioComponent>()) {
        m_SelectionContext.GetComponent<Heron::AudioComponent>().Play = (state == Qt::Checked);
    }
}

void PropertiesPanel::OnAudioShuffleChanged(int state)
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::AudioComponent>()) {
        m_SelectionContext.GetComponent<Heron::AudioComponent>().Shuffle = (state == Qt::Checked);
    }
}

void PropertiesPanel::OnAudioLoopChanged(int state)
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::AudioComponent>()) {
        m_SelectionContext.GetComponent<Heron::AudioComponent>().Loop = (state == Qt::Checked);
    }
}

// ===================== Terrain Slots =====================

void PropertiesPanel::OnAddTerrainComponent()
{
    if (m_SelectionContext && !m_SelectionContext.HasComponent<Heron::TerrainComponent>()) {
        m_SelectionContext.AddComponent<Heron::TerrainComponent>();
        DrawComponents();
    }
}

void PropertiesPanel::OnTerrainHeightmapClicked()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::TerrainComponent>()) {
        QString fileName = QFileDialog::getOpenFileName(this, "Select Heightmap Image", "", "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!fileName.isEmpty()) {
            auto& terrain = m_SelectionContext.GetComponent<Heron::TerrainComponent>();
            terrain.HeightmapPath = fileName.toStdString();
            terrain.Mesh = nullptr; // Invalidate mesh so it regenerates
            DrawComponents();
        }
    }
}

void PropertiesPanel::OnTerrainTextureClicked()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::TerrainComponent>()) {
        QString fileName = QFileDialog::getOpenFileName(this, "Select Terrain Texture", "", "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!fileName.isEmpty()) {
            auto& terrain = m_SelectionContext.GetComponent<Heron::TerrainComponent>();
            terrain.TexturePath = fileName.toStdString();
            terrain.Texture = nullptr; // Invalidate texture so it reloads
            DrawComponents();
        }
    }
}

void PropertiesPanel::OnTerrainValuesChanged()
{
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::TerrainComponent>()) {
        auto& terrain = m_SelectionContext.GetComponent<Heron::TerrainComponent>();
        float oldSize = terrain.Size;
        float oldHeight = terrain.HeightScale;
        int oldRes = terrain.Resolution;
        
        terrain.Size = m_TerrainSize->text().toFloat();
        terrain.HeightScale = m_TerrainHeightScale->text().toFloat();
        terrain.Resolution = m_TerrainResolution->text().toInt();
        
        // Ensure valid constraints
        if (terrain.Size < 1.0f) terrain.Size = 1.0f;
        if (terrain.Resolution < 2) terrain.Resolution = 2;
        if (terrain.Resolution > 1000) terrain.Resolution = 1000;
        
        // Only invalidate mesh if shape parameters changed
        if (oldSize != terrain.Size || oldHeight != terrain.HeightScale || oldRes != terrain.Resolution) {
            terrain.Mesh = nullptr;
        }
    }
}

