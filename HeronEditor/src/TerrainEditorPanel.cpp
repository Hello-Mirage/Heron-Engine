#include "TerrainEditorPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <Heron/Scene/Components.h>

TerrainEditorPanel::TerrainEditorPanel(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);

    // --- Tools Group ---
    QGroupBox* toolsGroup = new QGroupBox("Sculpting Tools", this);
    QVBoxLayout* toolsLayout = new QVBoxLayout(toolsGroup);

    m_ToolGroup = new QButtonGroup(this);
    m_ToolGroup->setExclusive(true);

    m_RaiseBtn = new QPushButton("Raise Height");
    m_RaiseBtn->setCheckable(true);
    m_LowerBtn = new QPushButton("Lower Height");
    m_LowerBtn->setCheckable(true);
    m_FlattenBtn = new QPushButton("Flatten");
    m_FlattenBtn->setCheckable(true);
    m_SmoothBtn = new QPushButton("Smooth");
    m_SmoothBtn->setCheckable(true);

    m_ToolGroup->addButton(m_RaiseBtn);
    m_ToolGroup->addButton(m_LowerBtn);
    m_ToolGroup->addButton(m_FlattenBtn);
    m_ToolGroup->addButton(m_SmoothBtn);

    m_RaiseBtn->setChecked(true); // Default

    toolsLayout->addWidget(m_RaiseBtn);
    toolsLayout->addWidget(m_LowerBtn);
    toolsLayout->addWidget(m_FlattenBtn);
    toolsLayout->addWidget(m_SmoothBtn);

    mainLayout->addWidget(toolsGroup);

    // --- Brush Properties Group ---
    QGroupBox* brushGroup = new QGroupBox("Brush Properties", this);
    QFormLayout* brushLayout = new QFormLayout(brushGroup);

    // Brush Size
    QHBoxLayout* sizeLayout = new QHBoxLayout();
    m_BrushSizeSlider = new QSlider(Qt::Horizontal);
    m_BrushSizeSlider->setRange(1, 100);
    m_BrushSizeSlider->setValue(20);
    
    m_BrushSizeSpinBox = new QSpinBox();
    m_BrushSizeSpinBox->setRange(1, 100);
    m_BrushSizeSpinBox->setValue(20);

    connect(m_BrushSizeSlider, &QSlider::valueChanged, m_BrushSizeSpinBox, &QSpinBox::setValue);
    connect(m_BrushSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), m_BrushSizeSlider, &QSlider::setValue);

    sizeLayout->addWidget(m_BrushSizeSlider);
    sizeLayout->addWidget(m_BrushSizeSpinBox);
    brushLayout->addRow("Size", sizeLayout);

    // Brush Strength
    QHBoxLayout* strengthLayout = new QHBoxLayout();
    m_BrushStrengthSlider = new QSlider(Qt::Horizontal);
    m_BrushStrengthSlider->setRange(1, 100); // 1 to 100 representing 0.01 to 1.0
    m_BrushStrengthSlider->setValue(50);

    m_BrushStrengthSpinBox = new QDoubleSpinBox();
    m_BrushStrengthSpinBox->setRange(0.01, 1.0);
    m_BrushStrengthSpinBox->setSingleStep(0.05);
    m_BrushStrengthSpinBox->setValue(0.5);

    connect(m_BrushStrengthSlider, &QSlider::valueChanged, this, [this](int value) {
        m_BrushStrengthSpinBox->setValue(value / 100.0);
    });
    connect(m_BrushStrengthSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        m_BrushStrengthSlider->setValue((int)(value * 100));
    });

    strengthLayout->addWidget(m_BrushStrengthSlider);
    strengthLayout->addWidget(m_BrushStrengthSpinBox);
    brushLayout->addRow("Strength", strengthLayout);

    mainLayout->addWidget(brushGroup);
    
    // Disable everything until an entity is selected
    this->setEnabled(false);
}

void TerrainEditorPanel::SetEntity(Heron::Entity entity)
{
    m_SelectionContext = entity;
    
    if (m_SelectionContext && m_SelectionContext.HasComponent<Heron::TerrainComponent>()) {
        this->setEnabled(true);
    } else {
        this->setEnabled(false);
    }
}

TerrainEditorPanel::BrushType TerrainEditorPanel::GetActiveBrush() const
{
    if (m_RaiseBtn->isChecked()) return BrushType::Raise;
    if (m_LowerBtn->isChecked()) return BrushType::Lower;
    if (m_FlattenBtn->isChecked()) return BrushType::Flatten;
    if (m_SmoothBtn->isChecked()) return BrushType::Smooth;
    return BrushType::Raise;
}

float TerrainEditorPanel::GetBrushSize() const
{
    return (float)m_BrushSizeSlider->value();
}

float TerrainEditorPanel::GetBrushStrength() const
{
    return (float)m_BrushStrengthSpinBox->value();
}
