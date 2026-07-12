#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QToolBar>
#include "SceneHierarchyPanel.h"
#include "PropertiesPanel.h"
#include "ViewportWidget.h"

class QFileSystemModel;
class QTreeView;

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

private:
    void SetupDocking();
    void CreateHierarchyPanel();
    void CreatePropertiesPanel();
    void CreateFileExplorerPanel();
    void CreateMenuBar();
    void CreateToolBar();

    ViewportWidget* m_Viewport;
    SceneHierarchyPanel* m_HierarchyPanel;
    PropertiesPanel* m_PropertiesPanel;
    QToolBar* m_ToolBar;

    QFileSystemModel* m_FileModel;
    QTreeView* m_FileTree;

    QString m_CurrentScenePath;
};
