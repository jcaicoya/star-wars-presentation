#include "StarsEditorWidget.h"
#include "StarMapWidget.h"

#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

const QString kEditorStyle = QStringLiteral(
    "QWidget { background: #10151b; color: #e0e8f0; font: 12pt 'Segoe UI'; }"
    "QListWidget { background: #161c24; border: 1px solid #2c3642; }"
    "QListWidget::item { padding: 6px 10px; }"
    "QListWidget::item:selected { background: #24384d; color: #f4fbff; }"
    "QLineEdit, QDoubleSpinBox {"
    "  background: #1a2230; border: 1px solid #2c3642; border-radius: 6px;"
    "  padding: 6px 10px; color: #e0e8f0;"
    "}"
    "QGroupBox { border: 1px solid #2c3642; border-radius: 8px; margin-top: 12px; padding-top: 18px; }"
    "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; color: #8899aa; }"
    "QToolButton {"
    "  background: #1c2836; border: 1px solid #2c3642; border-radius: 8px;"
    "  padding: 6px 14px; color: #d0dae4;"
    "}"
    "QToolButton:hover { background: #24384d; }"
    "QPushButton { border: 1px solid #2c3642; border-radius: 6px; padding: 6px 14px; }"
);

} // namespace

StarsEditorWidget::StarsEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(kEditorStyle);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(splitter);

    auto *leftPanel = new QWidget(this);
    buildLeftPanel(leftPanel);
    splitter->addWidget(leftPanel);

    m_starMap = new StarMapWidget(this);
    splitter->addWidget(m_starMap);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({340, 600});

    connect(m_starMap, &StarMapWidget::starSelected, this, [this](int index) {
        selectStar(index);
    });

    connect(m_starMap, &StarMapWidget::starMoved, this, [this](int index, const QVector3D &pos) {
        if (index < 0 || index >= static_cast<int>(m_stars.size()))
            return;
        m_stars[index].position = pos;
        if (index == m_selectedIndex)
            updatePropertyForm();
        setModified(true);
    });
}

// ── Public interface ─────────────────────────────────────────────────────────

void StarsEditorWidget::setStars(const std::vector<StarDefinition> &stars) {
    m_stars = stars;
    m_selectedIndex = -1;
    m_starMap->setStars(stars);
    syncListFromData();
    updatePropertyForm();
    setModified(false);
}

std::vector<StarDefinition> StarsEditorWidget::stars() const {
    return m_stars;
}

void StarsEditorWidget::setModified(const bool modified) {
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit modificationChanged(modified);
}

// ── UI construction ──────────────────────────────────────────────────────────

void StarsEditorWidget::buildLeftPanel(QWidget *parent) {
    auto *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(12, 12, 4, 12);
    layout->setSpacing(8);

    auto *listLabel = new QLabel(QStringLiteral("Stars"), parent);
    listLabel->setStyleSheet(QStringLiteral("font: 600 13pt 'Segoe UI'; color: #8899aa;"));
    layout->addWidget(listLabel);

    m_starList = new QListWidget(parent);
    layout->addWidget(m_starList, 1);

    connect(m_starList, &QListWidget::currentRowChanged, this, [this](int row) {
        selectStar(row);
    });

    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);
    m_addButton = new QToolButton(parent);
    m_addButton->setText(QStringLiteral("+ Add"));
    m_removeButton = new QToolButton(parent);
    m_removeButton->setText(QStringLiteral("- Remove"));
    buttonRow->addWidget(m_addButton);
    buttonRow->addWidget(m_removeButton);
    buttonRow->addStretch();
    layout->addLayout(buttonRow);

    connect(m_addButton, &QToolButton::clicked, this, &StarsEditorWidget::addStar);
    connect(m_removeButton, &QToolButton::clicked, this, &StarsEditorWidget::removeStar);

    m_propertyForm = new QWidget(parent);
    buildPropertyForm(m_propertyForm);
    layout->addWidget(m_propertyForm);
    m_propertyForm->setVisible(false);
}

void StarsEditorWidget::buildPropertyForm(QWidget *parent) {
    auto *group = new QGroupBox(QStringLiteral("Properties"), parent);
    auto *form = new QFormLayout(group);
    form->setContentsMargins(12, 16, 12, 12);
    form->setSpacing(8);

    m_nameEdit = new QLineEdit(group);
    m_nameEdit->setPlaceholderText(QStringLiteral("Star message text"));
    form->addRow(QStringLiteral("Text"), m_nameEdit);

    m_coreColorBtn = new QPushButton(group);
    m_coreColorBtn->setFixedHeight(28);
    form->addRow(QStringLiteral("Core color"), m_coreColorBtn);

    m_glowColorBtn = new QPushButton(group);
    m_glowColorBtn->setFixedHeight(28);
    form->addRow(QStringLiteral("Glow color"), m_glowColorBtn);

    m_radiusSpin = new QDoubleSpinBox(group);
    m_radiusSpin->setRange(0.1, 50.0);
    m_radiusSpin->setSingleStep(0.5);
    m_radiusSpin->setDecimals(1);
    form->addRow(QStringLiteral("Radius"), m_radiusSpin);

    m_posXSpin = new QDoubleSpinBox(group);
    m_posXSpin->setRange(-640.0, 640.0);
    m_posXSpin->setSingleStep(10.0);
    m_posXSpin->setDecimals(0);
    form->addRow(QStringLiteral("Position X"), m_posXSpin);

    m_posYSpin = new QDoubleSpinBox(group);
    m_posYSpin->setRange(-520.0, 440.0);
    m_posYSpin->setSingleStep(10.0);
    m_posYSpin->setDecimals(0);
    form->addRow(QStringLiteral("Position Y"), m_posYSpin);

    m_posZSpin = new QDoubleSpinBox(group);
    m_posZSpin->setRange(-1000.0, 2360.0);
    m_posZSpin->setSingleStep(10.0);
    m_posZSpin->setDecimals(0);
    form->addRow(QStringLiteral("Position Z"), m_posZSpin);

    auto *outerLayout = new QVBoxLayout(parent);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(group);

    // Connect property edits
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { applyPropertyToStar(); });
    connect(m_radiusSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() { applyPropertyToStar(); });
    connect(m_posXSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() { applyPropertyToStar(); });
    connect(m_posYSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() { applyPropertyToStar(); });
    connect(m_posZSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() { applyPropertyToStar(); });
    connect(m_coreColorBtn, &QPushButton::clicked, this, [this]() { onColorButtonClicked(true); });
    connect(m_glowColorBtn, &QPushButton::clicked, this, [this]() { onColorButtonClicked(false); });
}

// ── Data synchronization ─────────────────────────────────────────────────────

void StarsEditorWidget::syncListFromData() {
    m_starList->blockSignals(true);
    m_starList->clear();
    for (int i = 0; i < static_cast<int>(m_stars.size()); ++i) {
        const QString label = m_stars[i].text.isEmpty()
            ? QStringLiteral("Star %1").arg(i + 1)
            : m_stars[i].text;
        m_starList->addItem(label);
    }
    if (m_selectedIndex >= 0 && m_selectedIndex < m_starList->count())
        m_starList->setCurrentRow(m_selectedIndex);
    m_starList->blockSignals(false);
}

void StarsEditorWidget::selectStar(const int index) {
    const int clamped = (index >= 0 && index < static_cast<int>(m_stars.size())) ? index : -1;
    if (clamped == m_selectedIndex)
        return;

    m_selectedIndex = clamped;

    m_starList->blockSignals(true);
    m_starList->setCurrentRow(m_selectedIndex);
    m_starList->blockSignals(false);

    m_starMap->setSelectedIndex(m_selectedIndex);
    updatePropertyForm();
}

void StarsEditorWidget::updatePropertyForm() {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_stars.size())) {
        m_propertyForm->setVisible(false);
        return;
    }

    m_updatingForm = true;
    m_propertyForm->setVisible(true);

    const StarDefinition &star = m_stars[m_selectedIndex];
    m_nameEdit->setText(star.text);
    updateColorButton(m_coreColorBtn, star.coreColor);
    updateColorButton(m_glowColorBtn, star.glowColor);
    m_radiusSpin->setValue(star.radius);
    m_posXSpin->setValue(static_cast<double>(star.position.x()));
    m_posYSpin->setValue(static_cast<double>(star.position.y()));
    m_posZSpin->setValue(static_cast<double>(star.position.z()));

    m_updatingForm = false;
}

void StarsEditorWidget::applyPropertyToStar() {
    if (m_updatingForm || m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_stars.size()))
        return;

    StarDefinition &star = m_stars[m_selectedIndex];
    star.text = m_nameEdit->text();
    star.radius = m_radiusSpin->value();
    star.position = QVector3D(
        static_cast<float>(m_posXSpin->value()),
        static_cast<float>(m_posYSpin->value()),
        static_cast<float>(m_posZSpin->value()));

    m_starMap->updateStar(m_selectedIndex, star);
    syncListFromData();
    setModified(true);
}

// ── Star add/remove ──────────────────────────────────────────────────────────

void StarsEditorWidget::addStar() {
    StarDefinition star;
    star.text = QStringLiteral("New star");
    star.position = QVector3D(0.0f, 0.0f, 500.0f);
    star.coreColor = QColor(255, 255, 255);
    star.glowColor = QColor(153, 204, 255);
    star.radius = 5.0;

    m_stars.push_back(star);
    m_starMap->setStars(m_stars);
    syncListFromData();
    selectStar(static_cast<int>(m_stars.size()) - 1);
    setModified(true);
}

void StarsEditorWidget::removeStar() {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_stars.size()))
        return;

    m_stars.erase(m_stars.begin() + m_selectedIndex);

    const int newIndex = m_stars.empty() ? -1 : std::min(m_selectedIndex, static_cast<int>(m_stars.size()) - 1);
    m_selectedIndex = -1;

    m_starMap->setStars(m_stars);
    syncListFromData();
    selectStar(newIndex);
    setModified(true);
}

// ── Color buttons ────────────────────────────────────────────────────────────

void StarsEditorWidget::onColorButtonClicked(const bool core) {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_stars.size()))
        return;

    StarDefinition &star = m_stars[m_selectedIndex];
    const QColor initial = core ? star.coreColor : star.glowColor;
    const QColor chosen = QColorDialog::getColor(initial, this, core ? QStringLiteral("Core Color") : QStringLiteral("Glow Color"));
    if (!chosen.isValid())
        return;

    if (core)
        star.coreColor = chosen;
    else
        star.glowColor = chosen;

    updateColorButton(core ? m_coreColorBtn : m_glowColorBtn, chosen);
    m_starMap->updateStar(m_selectedIndex, star);
    setModified(true);
}

void StarsEditorWidget::updateColorButton(QPushButton *button, const QColor &color) {
    button->setStyleSheet(
        QStringLiteral("QPushButton { background: %1; border: 1px solid #2c3642; border-radius: 6px; }")
            .arg(color.name()));
    button->setText(color.name());
}
