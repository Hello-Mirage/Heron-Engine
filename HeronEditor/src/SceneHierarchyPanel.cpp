#include "SceneHierarchyPanel.h"
#include <Heron/Scene/Components.h>
#include <Heron/Scene/Components.h>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
SceneHierarchyPanel::SceneHierarchyPanel(QWidget* parent)
    : QDockWidget("Scene Hierarchy", parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_TreeWidget = new QTreeWidget(this);
    m_TreeWidget->setHeaderHidden(true);
    m_TreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    setWidget(m_TreeWidget);

    connect(m_TreeWidget, &QTreeWidget::itemSelectionChanged, this, &SceneHierarchyPanel::OnSelectionChanged);
    connect(m_TreeWidget, &QTreeWidget::customContextMenuRequested, this, &SceneHierarchyPanel::ShowContextMenu);
}

void SceneHierarchyPanel::SetContext(Heron::Scene* scene)
{
    m_Context = scene;
    m_SelectionContext = {};
    emit EntitySelected(m_SelectionContext);
    UpdateHierarchy();
}

void SceneHierarchyPanel::UpdateHierarchy()
{
    m_TreeWidget->clear();
    if (!m_Context) return;

    auto& registry = m_Context->GetRegistry();
    auto view = registry.view<Heron::TagComponent>();

    for (auto entityID : view) {
        Heron::Entity entity{ entityID, m_Context };
        auto& tag = entity.GetComponent<Heron::TagComponent>().Tag;
        
        QTreeWidgetItem* item = new QTreeWidgetItem(m_TreeWidget);
        item->setText(0, QString::fromStdString(tag));
        // Store entity ID in the item data so we can retrieve it on click
        item->setData(0, Qt::UserRole, QVariant((unsigned int)entityID));
    }
}

void SceneHierarchyPanel::OnSelectionChanged()
{
    auto selectedItems = m_TreeWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        m_SelectionContext = {};
        emit EntitySelected(m_SelectionContext);
        return;
    }

    entt::entity entityID = (entt::entity)selectedItems.first()->data(0, Qt::UserRole).toUInt();
    m_SelectionContext = Heron::Entity{ entityID, m_Context };
    emit EntitySelected(m_SelectionContext);
}

void SceneHierarchyPanel::ShowContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = m_TreeWidget->itemAt(pos);
    if (!item || !m_Context) return;

    QMenu menu(this);
    QAction* deleteAction = menu.addAction("Delete Entity");

    connect(deleteAction, &QAction::triggered, this, [this, item]() {
        entt::entity entityID = (entt::entity)item->data(0, Qt::UserRole).toUInt();
        Heron::Entity entity{ entityID, m_Context };
        
        m_Context->DestroyEntity(entity);
        
        if (m_SelectionContext == entity) {
            m_SelectionContext = {};
            emit EntitySelected(m_SelectionContext);
        }
        
        UpdateHierarchy();
    });

    menu.exec(m_TreeWidget->viewport()->mapToGlobal(pos));
}
