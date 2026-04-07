#pragma once

#include <QWidget>
#include <QVector3D>
#include <vector>

#include "CrawlContent.h"

class StarMapWidget final : public QWidget {
    Q_OBJECT

public:
    explicit StarMapWidget(QWidget *parent = nullptr);

    void setStars(const std::vector<StarDefinition> &stars);
    void setSelectedIndex(int index);
    int  selectedIndex() const { return m_selectedIndex; }

    const std::vector<StarDefinition> &stars() const { return m_stars; }
    void updateStar(int index, const StarDefinition &star);

signals:
    void starSelected(int index);
    void starMoved(int index, QVector3D position);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPointF projectToScreen(const QVector3D &worldPos) const;
    int     hitTest(const QPointF &screenPos) const;
    QVector3D unprojectDelta(const QPointF &screenDelta, float z) const;

    std::vector<StarDefinition> m_stars;
    int    m_selectedIndex = -1;
    bool   m_dragging      = false;
    QPoint m_dragStart;

    // Cube rendering constants
    static constexpr float kSpaceMinX = -640.0f;
    static constexpr float kSpaceMaxX =  640.0f;
    static constexpr float kSpaceMinY = -520.0f;
    static constexpr float kSpaceMaxY =  440.0f;
    static constexpr float kSpaceMinZ = -1000.0f;
    static constexpr float kSpaceMaxZ =  2360.0f;

    static constexpr float kMoveStep = 20.0f;
    static constexpr float kWheelStep = 30.0f;
    static constexpr qreal kHitRadius = 12.0;
};
