#pragma once

#include <QDockWidget>
#include <QTreeWidget>
#include <Heron/Scene/Scene.h>
#include <Heron/Scene/Entity.h>

class SceneHierarchyPanel : public QDockWidget
{
    Q_OBJECT

public:
    SceneHierarchyPanel(QWidget* parent = nullptr);
    ~SceneHierarchyPanel() = default;

    void SetContext(Heron::Scene* scene);
    Heron::Entity GetSelectedEntity() const { return m_SelectionContext; }

signals:
    void EntitySelected(Heron::Entity entity);

private slots:
    void OnSelectionChanged();
    void ShowContextMenu(const QPoint& pos);

public slots:
    void UpdateHierarchy();

private:
    QTreeWidget* m_TreeWidget;
    Heron::Scene* m_Context = nullptr;
    Heron::Entity m_SelectionContext;
};
