#pragma once

#include <QWidget>
#include <vector>

#include "CrawlContent.h"

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QToolButton;
class StarMapWidget;

class StarsEditorWidget final : public QWidget {
    Q_OBJECT

public:
    explicit StarsEditorWidget(QWidget *parent = nullptr);

    void setStars(const std::vector<StarDefinition> &stars);
    std::vector<StarDefinition> stars() const;

    bool isModified() const { return m_modified; }
    void setModified(bool modified);

signals:
    void modificationChanged(bool modified);

private:
    void buildLeftPanel(QWidget *parent);
    void buildPropertyForm(QWidget *parent);
    void syncListFromData();
    void selectStar(int index);
    void updatePropertyForm();
    void applyPropertyToStar();
    void addStar();
    void removeStar();
    void onColorButtonClicked(bool core);
    void updateColorButton(QPushButton *button, const QColor &color);

    StarMapWidget  *m_starMap       = nullptr;
    QListWidget    *m_starList      = nullptr;
    QToolButton    *m_addButton     = nullptr;
    QToolButton    *m_removeButton  = nullptr;

    // Property form
    QWidget        *m_propertyForm  = nullptr;
    QLineEdit      *m_nameEdit     = nullptr;
    QPushButton    *m_coreColorBtn = nullptr;
    QPushButton    *m_glowColorBtn = nullptr;
    QComboBox      *m_typeCombo    = nullptr;
    QDoubleSpinBox *m_radiusSpin   = nullptr;
    QDoubleSpinBox *m_posXSpin     = nullptr;
    QDoubleSpinBox *m_posYSpin     = nullptr;
    QDoubleSpinBox *m_posZSpin     = nullptr;

    std::vector<StarDefinition> m_stars;
    int  m_selectedIndex = -1;
    bool m_modified      = false;
    bool m_updatingForm  = false;

    // Space coordinate bounds (mirrored from StarMapWidget)
    static constexpr float kSpaceMinX = -640.0f;
    static constexpr float kSpaceMaxX =  640.0f;
    static constexpr float kSpaceMinY = -520.0f;
    static constexpr float kSpaceMaxY =  440.0f;
    static constexpr float kSpaceMinZ = -1000.0f;
    static constexpr float kSpaceMaxZ =  2360.0f;
    static constexpr float kEditorMinZ = 0.0f;
};
