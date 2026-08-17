#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QToolBar>
#include "SceneHierarchyPanel.h"
#include "PropertiesPanel.h"
#include "ViewportWidget.h"

class QFileSystemModel;
class QTreeView;
class QTabBar;

class EditorWindow : public QMainWindow

{
    Q_OBJECT

public:
    EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow();

private slots:
    void OnNewProject();
    void OnOpenProject();
    void OnNewScene();
    void OnOpenScene();
    void OnSaveScene();
    void OnSaveSceneAs();

    void OnCompile();

    void OnToggleMoveGizmo();
    void OnToggleRotateGizmo();
    void OnToggleScaleGizmo();

private:
    void SetupDocking();
    void CreateHierarchyPanel();
    void CreatePropertiesPanel();
    void CreateFileExplorerPanel();
    void CreateMenuBar();
    void CreateToolBar();

    void LoadGameDLL();
    void UnloadGameDLL();

    ViewportWidget* m_Viewport;
    QTabBar* m_ViewTabBar;
    SceneHierarchyPanel* m_HierarchyPanel;
    PropertiesPanel* m_PropertiesPanel;
    QToolBar* m_ToolBar;

    QAction* m_ActionMove;
    QAction* m_ActionRotate;
    QAction* m_ActionScale;

    QFileSystemModel* m_FileModel;
    QTreeView* m_FileTree;

    QString m_CurrentScenePath;
    void* m_GameDLLModule = nullptr;
};
