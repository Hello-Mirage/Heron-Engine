#include "SceneHierarchyPanel.h"
#include <Heron/Scene/Components.h>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include <QDropEvent>

class HierarchyTreeWidget : public QTreeWidget {
public:
    std::function<void(entt::entity, entt::entity)> OnEntityDropped;

    HierarchyTreeWidget(QWidget* parent = nullptr) : QTreeWidget(parent) {}

protected:
    void dropEvent(QDropEvent* event) override {
        QTreeWidgetItem* draggedItem = currentItem();
        QTreeWidgetItem* targetItem = itemAt(event->pos());

        if (draggedItem && targetItem && draggedItem != targetItem) {
            entt::entity dragged = (entt::entity)draggedItem->data(0, Qt::UserRole).toUInt();
            entt::entity target = (entt::entity)targetItem->data(0, Qt::UserRole).toUInt();
            if (OnEntityDropped) OnEntityDropped(dragged, target);
        } else if (draggedItem && !targetItem) {
            entt::entity dragged = (entt::entity)draggedItem->data(0, Qt::UserRole).toUInt();
            if (OnEntityDropped) OnEntityDropped(dragged, entt::null);
        }
        event->accept();
    }
};

SceneHierarchyPanel::SceneHierarchyPanel(QWidget* parent)
    : QDockWidget("Scene Hierarchy", parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    HierarchyTreeWidget* treeWidget = new HierarchyTreeWidget(this);
    treeWidget->setHeaderHidden(true);
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    treeWidget->setDragEnabled(true);
    treeWidget->setAcceptDrops(true);
    treeWidget->setDropIndicatorShown(true);
    treeWidget->setDragDropMode(QAbstractItemView::InternalMove);
    
    treeWidget->OnEntityDropped = [this](entt::entity dragged, entt::entity target) {
        if (!m_Context) return;
        
        Heron::Entity draggedEnt{ dragged, m_Context };
        Heron::Entity targetEnt;
        if (target != entt::null) targetEnt = Heron::Entity{ target, m_Context };

        // Ensure not dropping parent onto its own child (cycle prevention)
        bool cycle = false;
        Heron::Entity check = targetEnt;
        while (check) {
            if (check == draggedEnt) { cycle = true; break; }
            if (check.HasComponent<Heron::RelationshipComponent>()) {
                auto p = check.GetComponent<Heron::RelationshipComponent>().Parent;
                if (p != entt::null) check = Heron::Entity{ p, m_Context };
                else break;
            } else { break; }
        }

        if (!cycle) {
            auto& draggedRel = draggedEnt.GetComponent<Heron::RelationshipComponent>();
            
            // Remove from old parent
            if (draggedRel.Parent != entt::null) {
                Heron::Entity oldParent{ draggedRel.Parent, m_Context };
                auto& oldParentRel = oldParent.GetComponent<Heron::RelationshipComponent>();
                oldParentRel.Children.erase(std::remove(oldParentRel.Children.begin(), oldParentRel.Children.end(), dragged), oldParentRel.Children.end());
            }

            // Set new parent
            draggedRel.Parent = target;
            if (target != entt::null) {
                auto& targetRel = targetEnt.GetComponent<Heron::RelationshipComponent>();
                targetRel.Children.push_back(dragged);
            }

            UpdateHierarchy();
        }
    };

    m_TreeWidget = treeWidget;
    setWidget(m_TreeWidget);

    connect(m_TreeWidget, &QTreeWidget::itemSelectionChanged, this, &SceneHierarchyPanel::OnSelectionChanged);
    connect(m_TreeWidget, &QTreeWidget::customContextMenuRequested, this, &SceneHierarchyPanel::ShowContextMenu);
}

void SceneHierarchyPanel::SetContext(Heron::Scene* scene)
{
    std::string previousSelectionTag = "";
    if (m_SelectionContext) {
        previousSelectionTag = m_SelectionContext.GetComponent<Heron::TagComponent>().Tag;
    }

    m_Context = scene;
    m_SelectionContext = {};
    
    UpdateHierarchy();

    if (!previousSelectionTag.empty() && m_Context) {
        auto view = m_Context->GetRegistry().view<Heron::TagComponent>();
        for (auto entityID : view) {
            if (view.get<Heron::TagComponent>(entityID).Tag == previousSelectionTag) {
                m_SelectionContext = Heron::Entity{ entityID, m_Context };
                break;
            }
        }
    }

    emit EntitySelected(m_SelectionContext);
    
    // Select it in the UI tree as well
    if (m_SelectionContext) {
        QList<QTreeWidgetItem*> items = m_TreeWidget->findItems(QString::fromStdString(previousSelectionTag), Qt::MatchExactly, 0);
        if (!items.isEmpty()) {
            m_TreeWidget->blockSignals(true);
            m_TreeWidget->clearSelection();
            items.first()->setSelected(true);
            m_TreeWidget->blockSignals(false);
        }
    }
}

void SceneHierarchyPanel::UpdateHierarchy()
{
    m_TreeWidget->clear();
    if (!m_Context) return;

    auto& registry = m_Context->GetRegistry();
    auto view = registry.view<Heron::TagComponent, Heron::RelationshipComponent>();

    std::function<void(entt::entity, QTreeWidgetItem*)> addChildren = [&](entt::entity parentID, QTreeWidgetItem* parentItem) {
        auto& rc = registry.get<Heron::RelationshipComponent>(parentID);
        for (auto childID : rc.Children) {
            Heron::Entity child{ childID, m_Context };
            auto& tag = child.GetComponent<Heron::TagComponent>().Tag;
            
            QTreeWidgetItem* item;
            if (parentItem) item = new QTreeWidgetItem(parentItem);
            else item = new QTreeWidgetItem(m_TreeWidget);
            
            item->setText(0, QString::fromStdString(tag));
            item->setData(0, Qt::UserRole, QVariant((unsigned int)childID));
            
            addChildren(childID, item);
        }
    };

    // Find all root entities (no parent) and add them
    for (auto entityID : view) {
        auto& rc = view.get<Heron::RelationshipComponent>(entityID);
        if (rc.Parent == entt::null) {
            Heron::Entity entity{ entityID, m_Context };
            auto& tag = entity.GetComponent<Heron::TagComponent>().Tag;
            
            QTreeWidgetItem* item = new QTreeWidgetItem(m_TreeWidget);
            item->setText(0, QString::fromStdString(tag));
            item->setData(0, Qt::UserRole, QVariant((unsigned int)entityID));
            
            addChildren(entityID, item);
        }
    }
    
    m_TreeWidget->expandAll();
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
    if (!m_Context) return;

    QTreeWidgetItem* item = m_TreeWidget->itemAt(pos);
    QMenu* menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QAction* createEmptyAction = menu->addAction("Create Empty Entity");
    connect(createEmptyAction, &QAction::triggered, this, [this]() {
        m_Context->CreateEntity("Empty Entity");
        UpdateHierarchy();
    });

    if (item) {
        menu->addSeparator();
        QAction* deleteAction = menu->addAction("Delete Entity");
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

        QAction* duplicateAction = menu->addAction("Duplicate Entity");
        connect(duplicateAction, &QAction::triggered, this, [this, item]() {
            entt::entity entityID = (entt::entity)item->data(0, Qt::UserRole).toUInt();
            Heron::Entity entity{ entityID, m_Context };
            
            m_Context->DuplicateEntity(entity);
            UpdateHierarchy();
        });
    }

    menu->popup(m_TreeWidget->viewport()->mapToGlobal(pos));
}
