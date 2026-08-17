#include "EditorWindow.h"
#include "ViewportWidget.h"
#include <QLabel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QFileSystemModel>
#include <QTreeView>
#include <QFileDialog>
#include <QInputDialog>
#include <QDir>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <Heron/Scene/SceneSerializer.h>
#include <windows.h>
#include <QProcess>
#include <QTabBar>

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Heron Engine - Editor");
    resize(1280, 720);

    // Setup Central Widget with Tab Bar
    QWidget* centralContainer = new QWidget(this);
    QVBoxLayout* centralLayout = new QVBoxLayout(centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    m_ViewTabBar = new QTabBar(centralContainer);
    m_ViewTabBar->addTab("Scene");
    m_ViewTabBar->addTab("Game");

    m_Viewport = new ViewportWidget(centralContainer);
    
    centralLayout->addWidget(m_ViewTabBar);
    centralLayout->addWidget(m_Viewport);

    setCentralWidget(centralContainer);

    connect(m_ViewTabBar, &QTabBar::currentChanged, this, [this](int index) {
        if (index == 0) {
            m_Viewport->SetViewMode(ViewportWidget::ViewMode::Scene);
        } else {
            m_Viewport->SetViewMode(ViewportWidget::ViewMode::Game);
        }
        m_Viewport->update();
    });

    CreateMenuBar();
    CreateToolBar();
    SetupDocking();

    // Link Viewport Scene to Hierarchy
    m_HierarchyPanel->SetContext(m_Viewport->GetScene());
    connect(m_Viewport, &ViewportWidget::EntitySpawned, m_HierarchyPanel, &SceneHierarchyPanel::UpdateHierarchy);
    connect(m_Viewport, &ViewportWidget::SceneChanged, m_HierarchyPanel, &SceneHierarchyPanel::SetContext);
}

EditorWindow::~EditorWindow()
{
}

void EditorWindow::SetupDocking()
{
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks);
    
    CreateHierarchyPanel();
    CreatePropertiesPanel();
    CreateFileExplorerPanel();
}

void EditorWindow::CreateHierarchyPanel()
{
    m_HierarchyPanel = new SceneHierarchyPanel(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_HierarchyPanel);
}

void EditorWindow::CreatePropertiesPanel()
{
    m_PropertiesPanel = new PropertiesPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_PropertiesPanel);

    // Connect selection in hierarchy to properties
    connect(m_HierarchyPanel, &SceneHierarchyPanel::EntitySelected,
            m_PropertiesPanel, &PropertiesPanel::SetEntity);

    connect(m_HierarchyPanel, &SceneHierarchyPanel::EntitySelected,
            m_Viewport, &ViewportWidget::SetSelectedEntity);

    connect(m_Viewport, &ViewportWidget::PropertiesChanged,
            m_PropertiesPanel, &PropertiesPanel::UpdateValues);
}

void EditorWindow::CreateFileExplorerPanel()
{
    QDockWidget* dock = new QDockWidget("Content Browser", this);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    m_FileModel = new QFileSystemModel(this);
    // Set default root to current path (so we don't scan all drives by default)
    QString currentPath = QDir::currentPath();
    m_FileModel->setRootPath(currentPath);

    m_FileTree = new QTreeView(dock);
    m_FileTree->setModel(m_FileModel);
    m_FileTree->setRootIndex(m_FileModel->index(currentPath));
    m_FileTree->setColumnWidth(0, 250);

    dock->setWidget(m_FileTree);

    addDockWidget(Qt::BottomDockWidgetArea, dock);
}

void EditorWindow::CreateMenuBar()
{
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // File Menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    
    QAction* newProjectAction = new QAction("New &Project...", this);
    QAction* openProjectAction = new QAction("&Open Project...", this);
    QAction* newSceneAction = new QAction("&New Scene", this);
    QAction* openSceneAction = new QAction("&Open Scene...", this);
    QAction* saveSceneAction = new QAction("&Save Scene", this);
    QAction* saveSceneAsAction = new QAction("Save Scene &As...", this);
    QAction* exitAction = new QAction("E&xit", this);
    
    connect(newProjectAction, &QAction::triggered, this, &EditorWindow::OnNewProject);
    connect(openProjectAction, &QAction::triggered, this, &EditorWindow::OnOpenProject);
    connect(newSceneAction, &QAction::triggered, this, &EditorWindow::OnNewScene);
    connect(openSceneAction, &QAction::triggered, this, &EditorWindow::OnOpenScene);
    connect(saveSceneAction, &QAction::triggered, this, &EditorWindow::OnSaveScene);
    connect(saveSceneAsAction, &QAction::triggered, this, &EditorWindow::OnSaveSceneAs);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    fileMenu->addAction(newProjectAction);
    fileMenu->addAction(openProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(newSceneAction);
    fileMenu->addAction(openSceneAction);
    fileMenu->addSeparator();
    fileMenu->addAction(saveSceneAction);
    fileMenu->addAction(saveSceneAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    // Edit Menu
    QMenu* editMenu = menuBar->addMenu("&Edit");
    editMenu->addAction(new QAction("&Undo", this));
    editMenu->addAction(new QAction("&Redo", this));
    editMenu->addSeparator();
    editMenu->addAction(new QAction("C&ut", this));
    editMenu->addAction(new QAction("&Copy", this));
    editMenu->addAction(new QAction("&Paste", this));

    // View Menu
    QMenu* viewMenu = menuBar->addMenu("&View");
    viewMenu->addAction(new QAction("Toggle &Fullscreen", this));
    
    // Help Menu
    QMenu* helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction(new QAction("&About Heron Engine", this));
}

void EditorWindow::CreateToolBar()
{
    m_ToolBar = addToolBar("Playback");
    
    QAction* saveAction = new QAction("💾 Save", this);
    QAction* compileAction = new QAction("🔨 Compile", this);
    QAction* playAction = new QAction("▶ Play", this);
    QAction* stopAction = new QAction("⏹ Stop", this);
    
    m_ToolBar->addAction(saveAction);
    m_ToolBar->addSeparator();
    m_ToolBar->addAction(compileAction);
    m_ToolBar->addSeparator();
    m_ToolBar->addAction(playAction);
    m_ToolBar->addAction(stopAction);

    connect(saveAction, &QAction::triggered, this, &EditorWindow::OnSaveScene);
    connect(compileAction, &QAction::triggered, this, &EditorWindow::OnCompile);

    connect(playAction, &QAction::triggered, this, [this]() {
        m_Viewport->SetSceneState(ViewportWidget::SceneState::Play);
    });

    connect(stopAction, &QAction::triggered, this, [this]() {
        m_Viewport->SetSceneState(ViewportWidget::SceneState::Edit);
    });

    m_ToolBar->addSeparator();

    m_ActionMove = new QAction("Move", this);
    m_ActionMove->setCheckable(true);
    m_ActionRotate = new QAction("Rotate", this);
    m_ActionRotate->setCheckable(true);
    m_ActionScale = new QAction("Scale", this);
    m_ActionScale->setCheckable(true);

    m_ToolBar->addAction(m_ActionMove);
    m_ToolBar->addAction(m_ActionRotate);
    m_ToolBar->addAction(m_ActionScale);

    connect(m_ActionMove, &QAction::triggered, this, &EditorWindow::OnToggleMoveGizmo);
    connect(m_ActionRotate, &QAction::triggered, this, &EditorWindow::OnToggleRotateGizmo);
    connect(m_ActionScale, &QAction::triggered, this, &EditorWindow::OnToggleScaleGizmo);
}

void EditorWindow::OnToggleMoveGizmo()
{
    if (m_ActionMove->isChecked()) {
        m_Viewport->SetGizmoMode(ViewportWidget::GizmoMode::Translate);
        m_ActionRotate->setChecked(false);
        m_ActionScale->setChecked(false);
    } else {
        m_Viewport->SetGizmoMode(ViewportWidget::GizmoMode::None);
    }
}

void EditorWindow::OnToggleRotateGizmo()
{
    if (m_ActionRotate->isChecked()) {
        m_Viewport->SetGizmoMode(ViewportWidget::GizmoMode::Rotate);
        m_ActionMove->setChecked(false);
        m_ActionScale->setChecked(false);
    } else {
        m_Viewport->SetGizmoMode(ViewportWidget::GizmoMode::None);
    }
}

void EditorWindow::OnToggleScaleGizmo()
{
    if (m_ActionScale->isChecked()) {
        m_Viewport->SetGizmoMode(ViewportWidget::GizmoMode::Scale);
        m_ActionMove->setChecked(false);
        m_ActionRotate->setChecked(false);
    } else {
        m_Viewport->SetGizmoMode(ViewportWidget::GizmoMode::None);
    }
}

void EditorWindow::OnNewProject()
{
    // Step 1: Select parent directory
    QString parentDir = QFileDialog::getExistingDirectory(this, "Select Directory to Create New Project", QDir::homePath());
    if (parentDir.isEmpty())
        return;

    // Step 2: Ask for project name
    bool ok;
    QString projectName = QInputDialog::getText(this, "New Project", "Project Name:", QLineEdit::Normal, "MyNewGame", &ok);
    
    if (!ok || projectName.isEmpty())
        return;

    // Step 3: Create directories
    QDir dir(parentDir);
    if (!dir.mkdir(projectName)) {
        QMessageBox::critical(this, "Error", "Failed to create project folder!");
        return;
    }

    dir.cd(projectName);
    dir.mkdir("assets");
    dir.mkdir("essentials");

    // Step 4: Update Content Browser
    QString projectPath = dir.absolutePath();
    m_FileModel->setRootPath(projectPath);
    m_FileTree->setRootIndex(m_FileModel->index(projectPath));

    // Step 5: Automatically create essentials/main.heron
    OnNewScene();
    m_CurrentScenePath = projectPath + "/essentials/main.heron";
    OnSaveScene();
}

void EditorWindow::OnOpenProject()
{
    QString projectDir = QFileDialog::getExistingDirectory(this, "Open Project Folder", QDir::homePath());
    if (projectDir.isEmpty())
        return;

    m_FileModel->setRootPath(projectDir);
    m_FileTree->setRootIndex(m_FileModel->index(projectDir));

    // Look for essentials/main.heron and load it automatically
    QString mainScenePath = projectDir + "/essentials/main.heron";
    if (QFile::exists(mainScenePath)) {
        OnNewScene(); // clear current scene
        Heron::SceneSerializer serializer(m_Viewport->GetSceneRef());
        serializer.Deserialize(mainScenePath.toStdString());
        m_CurrentScenePath = mainScenePath;
        m_HierarchyPanel->SetContext(m_Viewport->GetScene());
        m_PropertiesPanel->SetEntity({});
    }
}

void EditorWindow::OnNewScene()
{
    if (m_Viewport->GetSceneState() == ViewportWidget::SceneState::Play)
        m_Viewport->SetSceneState(ViewportWidget::SceneState::Edit);
        
    auto newScene = Heron::CreateRef<Heron::Scene>();
    m_Viewport->SetScene(newScene);
    m_HierarchyPanel->SetContext(newScene.get());
    m_PropertiesPanel->SetEntity({});
    m_CurrentScenePath = QString();
}

void EditorWindow::OnOpenScene()
{
    QString filepath = QFileDialog::getOpenFileName(this, "Open Scene", QDir::currentPath(), "Heron Scene (*.heron)");
    if (!filepath.isEmpty()) {
        if (m_Viewport->GetSceneState() == ViewportWidget::SceneState::Play)
            m_Viewport->SetSceneState(ViewportWidget::SceneState::Edit);
            
        OnNewScene(); // clear current scene
        Heron::SceneSerializer serializer(m_Viewport->GetSceneRef());
        serializer.Deserialize(filepath.toStdString());
        m_CurrentScenePath = filepath;
        m_HierarchyPanel->SetContext(m_Viewport->GetScene());
        m_PropertiesPanel->SetEntity({});
    }
}

void EditorWindow::OnSaveScene()
{
    if (m_CurrentScenePath.isEmpty()) {
        OnSaveSceneAs();
    } else {
        Heron::SceneSerializer serializer(m_Viewport->GetSceneRef());
        serializer.Serialize(m_CurrentScenePath.toStdString());
    }
}

void EditorWindow::OnSaveSceneAs()
{
    QString filepath = QFileDialog::getSaveFileName(this, "Save Scene As", QDir::currentPath(), "Heron Scene (*.heron)");
    if (!filepath.isEmpty()) {
        m_CurrentScenePath = filepath;
        Heron::SceneSerializer serializer(m_Viewport->GetSceneRef());
        serializer.Serialize(m_CurrentScenePath.toStdString());
    }
}

void EditorWindow::OnCompile()
{
    HRN_CORE_INFO("Compiling GameProject...");
    
    // Stop playback if playing
    if (m_Viewport->GetSceneState() == ViewportWidget::SceneState::Play) {
        m_Viewport->SetSceneState(ViewportWidget::SceneState::Edit);
    }
    
    UnloadGameDLL();
    
    QProcess process;
    process.setWorkingDirectory(QDir::currentPath());
    process.start("cmake", QStringList() << "--build" << "build" << "--config" << "Debug" << "--target" << "GameProject");
    
    process.waitForFinished(-1);
    
    if (process.exitCode() == 0) {
        HRN_CORE_INFO("Compilation successful. Reloading DLL...");
        LoadGameDLL();
        
        // Refresh scene
        if (!m_CurrentScenePath.isEmpty()) {
            OnSaveScene(); // Save current state
            OnOpenScene(); // Reload to pick up new scripts
        }
    } else {
        HRN_CORE_ERROR("Compilation failed!");
        HRN_CORE_ERROR("{0}", process.readAllStandardError().toStdString());
    }
}

void EditorWindow::LoadGameDLL()
{
    QString dllPath = QDir::currentPath() + "/build/bin/Debug/GameProject.dll";
    QString copyPath = QDir::currentPath() + "/build/bin/Debug/GameProject_temp.dll";
    
    if (!QFile::exists(dllPath)) {
        HRN_CORE_WARN("GameProject.dll not found at {0}", dllPath.toStdString());
        return;
    }
    
    QFile::remove(copyPath);
    if (QFile::copy(dllPath, copyPath)) {
        m_GameDLLModule = (void*)LoadLibraryA(copyPath.toStdString().c_str());
        if (m_GameDLLModule) {
            HRN_CORE_INFO("Loaded Game DLL from {0}", copyPath.toStdString());
            
            // Execute InitializeGame hook
            typedef void(*InitGameFn)();
            InitGameFn initFn = (InitGameFn)GetProcAddress((HMODULE)m_GameDLLModule, "InitializeGame");
            if (initFn) {
                initFn();
            } else {
                HRN_CORE_WARN("InitializeGame not found in Game DLL");
            }
        } else {
            HRN_CORE_ERROR("Failed to load Game DLL");
        }
    } else {
        HRN_CORE_ERROR("Failed to copy Game DLL");
    }
}

void EditorWindow::UnloadGameDLL()
{
    if (m_GameDLLModule) {
        FreeLibrary((HMODULE)m_GameDLLModule);
        m_GameDLLModule = nullptr;
        HRN_CORE_INFO("Unloaded Game DLL");
    }
}
