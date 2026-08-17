#pragma once

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QButtonGroup>
#include "Heron/Scene/Entity.h"

class TerrainEditorPanel : public QWidget
{
    Q_OBJECT

public:
    TerrainEditorPanel(QWidget* parent = nullptr);
    ~TerrainEditorPanel() = default;

    void SetEntity(Heron::Entity entity);

    enum class BrushType {
        Raise, Lower, Flatten, Smooth
    };

    BrushType GetActiveBrush() const;
    float GetBrushSize() const;
    float GetBrushStrength() const;

signals:
    void ExitRequested();

private:
    Heron::Entity m_SelectionContext;

    QButtonGroup* m_ToolGroup;
    QPushButton* m_RaiseBtn;
    QPushButton* m_LowerBtn;
    QPushButton* m_FlattenBtn;
    QPushButton* m_SmoothBtn;

    QSlider* m_BrushSizeSlider;
    QSpinBox* m_BrushSizeSpinBox;

    QSlider* m_BrushStrengthSlider;
    QDoubleSpinBox* m_BrushStrengthSpinBox;

    QPushButton* m_ExitBtn;
};
