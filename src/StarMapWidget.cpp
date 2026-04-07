#include "StarMapWidget.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {

constexpr qreal kDepthRatio    = 0.22;
constexpr qreal kCubeMargin    = 48.0;
constexpr qreal kStarDotRadius = 5.0;
constexpr qreal kSelectedDotRadius = 7.0;

} // namespace

StarMapWidget::StarMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(300, 300);
    setStyleSheet(QStringLiteral("background: #0a0e14;"));
}

void StarMapWidget::setStars(const std::vector<StarDefinition> &stars) {
    m_stars = stars;
    if (m_selectedIndex >= static_cast<int>(m_stars.size()))
        m_selectedIndex = -1;
    update();
}

void StarMapWidget::setSelectedIndex(const int index) {
    if (index == m_selectedIndex)
        return;
    m_selectedIndex = (index >= 0 && index < static_cast<int>(m_stars.size())) ? index : -1;
    update();
}

void StarMapWidget::updateStar(const int index, const StarDefinition &star) {
    if (index < 0 || index >= static_cast<int>(m_stars.size()))
        return;
    m_stars[index] = star;
    update();
}

// ── Projection ───────────────────────────────────────────────────────────────

QPointF StarMapWidget::projectToScreen(const QVector3D &worldPos) const {
    const qreal side = std::min(width(), height()) - kCubeMargin * 2.0;
    const qreal depthExtent = side * kDepthRatio;
    const qreal cubeSize = side - depthExtent;
    const qreal cubeLeft = (width() - side) * 0.5;
    const qreal cubeTop  = (height() - side) * 0.5 + depthExtent;

    const qreal nx = (worldPos.x() - kSpaceMinX) / (kSpaceMaxX - kSpaceMinX);
    const qreal ny = (worldPos.y() - kSpaceMinY) / (kSpaceMaxY - kSpaceMinY);
    const qreal nz = (worldPos.z() - kEditorMinZ) / (kSpaceMaxZ - kEditorMinZ);

    return QPointF(
        cubeLeft + cubeSize * nx + depthExtent * nz,
        cubeTop + cubeSize * (1.0 - ny) - depthExtent * nz);
}

QVector3D StarMapWidget::unprojectDelta(const QPointF &screenDelta, const float z) const {
    const qreal side = std::min(width(), height()) - kCubeMargin * 2.0;
    const qreal cubeSize = side - side * kDepthRatio;
    if (cubeSize < 1.0)
        return QVector3D();

    const float dx = static_cast<float>(screenDelta.x() / cubeSize) * (kSpaceMaxX - kSpaceMinX);
    const float dy = static_cast<float>(-screenDelta.y() / cubeSize) * (kSpaceMaxY - kSpaceMinY);
    return QVector3D(dx, dy, z);
}

int StarMapWidget::hitTest(const QPointF &screenPos) const {
    int bestIndex = -1;
    qreal bestDist = kHitRadius;

    for (int i = 0; i < static_cast<int>(m_stars.size()); ++i) {
        const QPointF projected = projectToScreen(m_stars[i].position);
        const qreal dist = std::hypot(projected.x() - screenPos.x(), projected.y() - screenPos.y());
        if (dist < bestDist) {
            bestDist = dist;
            bestIndex = i;
        }
    }
    return bestIndex;
}

// ── Painting ─────────────────────────────────────────────────────────────────

void StarMapWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), QColor(10, 14, 20));

    // ── Cube geometry ────────────────────────────────────────────────────────
    const qreal side = std::min(width(), height()) - kCubeMargin * 2.0;
    const qreal depthExtent = side * kDepthRatio;
    const qreal cubeSize = side - depthExtent;
    const qreal cubeLeft = (width() - side) * 0.5;
    const qreal cubeTop  = (height() - side) * 0.5 + depthExtent;

    const QRectF frontRect(cubeLeft, cubeTop, cubeSize, cubeSize);
    const QPointF depthOffset(depthExtent, -depthExtent);
    const QRectF backRect = frontRect.translated(depthOffset);

    // ── Draw cube wireframe ──────────────────────────────────────────────────
    const QColor cubeColor(120, 255, 150, 120);
    painter.setBrush(QColor(20, 60, 28, 20));
    painter.setPen(QPen(cubeColor, 1.0));
    painter.drawRect(frontRect);
    painter.drawRect(backRect);
    painter.drawLine(frontRect.topLeft(), backRect.topLeft());
    painter.drawLine(frontRect.topRight(), backRect.topRight());
    painter.drawLine(frontRect.bottomLeft(), backRect.bottomLeft());
    painter.drawLine(frontRect.bottomRight(), backRect.bottomRight());

    // ── Axis labels with coordinate values ───────────────────────────────────
    const QColor labelColor(120, 255, 150, 140);
    const QColor valueColor(120, 255, 150, 100);
    const QFont labelFont(QStringLiteral("Consolas"), 9, QFont::DemiBold);
    const QFont valueFont(QStringLiteral("Consolas"), 8);

    // X axis — along bottom edge of front face
    painter.setFont(labelFont);
    painter.setPen(labelColor);
    painter.drawText(QPointF(frontRect.center().x() - 4.0, frontRect.bottom() + 28.0), QStringLiteral("X"));
    painter.setFont(valueFont);
    painter.setPen(valueColor);
    painter.drawText(QPointF(frontRect.left() - 4.0, frontRect.bottom() + 16.0), QStringLiteral("0%"));
    painter.drawText(QPointF(frontRect.right() - 20.0, frontRect.bottom() + 16.0), QStringLiteral("100%"));

    // Y axis — along left edge of front face
    painter.setFont(labelFont);
    painter.setPen(labelColor);
    painter.drawText(QPointF(frontRect.left() - 28.0, frontRect.center().y() + 4.0), QStringLiteral("Y"));
    painter.setFont(valueFont);
    painter.setPen(valueColor);
    painter.drawText(QPointF(frontRect.left() - 22.0, frontRect.bottom() + 4.0), QStringLiteral("0%"));
    painter.drawText(QPointF(frontRect.left() - 32.0, frontRect.top() + 12.0), QStringLiteral("100%"));

    // Z axis — along the diagonal depth edge (bottom-left front to bottom-left back)
    painter.setFont(labelFont);
    painter.setPen(labelColor);
    const QPointF zMid = (frontRect.bottomRight() + backRect.bottomRight()) * 0.5;
    painter.drawText(QPointF(zMid.x() + 8.0, zMid.y() + 4.0), QStringLiteral("Z"));
    painter.setFont(valueFont);
    painter.setPen(valueColor);
    painter.drawText(QPointF(frontRect.bottomRight().x() + 6.0, frontRect.bottomRight().y() + 4.0), QStringLiteral("0%"));
    painter.drawText(QPointF(backRect.bottomRight().x() + 6.0, backRect.bottomRight().y() + 4.0), QStringLiteral("100%"));

    // ── Draw projection lines for all stars (behind the star dots) ─────────
    for (int i = 0; i < static_cast<int>(m_stars.size()); ++i) {
        const StarDefinition &star = m_stars[i];
        const QPointF sp = projectToScreen(star.position);
        const bool selected = (i == m_selectedIndex);
        const int lineAlpha = selected ? 90 : 40;

        // Project to all 6 walls
        const QPointF pXMin = projectToScreen(QVector3D(kSpaceMinX, star.position.y(), star.position.z()));
        const QPointF pXMax = projectToScreen(QVector3D(kSpaceMaxX, star.position.y(), star.position.z()));
        const QPointF pYMin = projectToScreen(QVector3D(star.position.x(), kSpaceMinY, star.position.z()));
        const QPointF pYMax = projectToScreen(QVector3D(star.position.x(), kSpaceMaxY, star.position.z()));
        const QPointF pZMin = projectToScreen(QVector3D(star.position.x(), star.position.y(), kEditorMinZ));
        const QPointF pZMax = projectToScreen(QVector3D(star.position.x(), star.position.y(), kSpaceMaxZ));

        QPen dashedPen(QColor(255, 255, 255, lineAlpha), 1.0, Qt::DotLine);
        painter.setPen(dashedPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(pXMin, pXMax);
        painter.drawLine(pYMin, pYMax);
        painter.drawLine(pZMin, pZMax);

        // Small markers at wall intersections
        const QColor markerColor(255, 255, 255, lineAlpha + 20);
        painter.setPen(Qt::NoPen);
        painter.setBrush(markerColor);
        for (const QPointF &wp : {pXMin, pXMax, pYMin, pYMax, pZMin, pZMax})
            painter.drawEllipse(wp, 2.0, 2.0);
    }

    // ── Draw stars ───────────────────────────────────────────────────────────
    for (int i = 0; i < static_cast<int>(m_stars.size()); ++i) {
        const StarDefinition &star = m_stars[i];
        const QPointF point = projectToScreen(star.position);
        const bool selected = (i == m_selectedIndex);

        if (selected) {
            painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(point, kSelectedDotRadius + 4.0, kSelectedDotRadius + 4.0);
        }

        // Glow
        painter.setPen(Qt::NoPen);
        QColor glow = star.glowColor;
        glow.setAlpha(selected ? 140 : 80);
        painter.setBrush(glow);
        painter.drawEllipse(point, (selected ? kSelectedDotRadius : kStarDotRadius) + 3.0,
                            (selected ? kSelectedDotRadius : kStarDotRadius) + 3.0);

        // Core
        painter.setBrush(star.coreColor);
        painter.drawEllipse(point, selected ? kSelectedDotRadius : kStarDotRadius,
                            selected ? kSelectedDotRadius : kStarDotRadius);

        // Label + percentage coordinates for all stars
        const qreal nx = static_cast<qreal>(star.position.x() - kSpaceMinX) / (kSpaceMaxX - kSpaceMinX) * 100.0;
        const qreal ny = static_cast<qreal>(star.position.y() - kSpaceMinY) / (kSpaceMaxY - kSpaceMinY) * 100.0;
        const qreal nz = static_cast<qreal>(star.position.z() - kEditorMinZ) / (kSpaceMaxZ - kEditorMinZ) * 100.0;

        const qreal labelX = point.x() + 10.0;
        qreal labelY = point.y() + 4.0;
        if (!star.text.isEmpty()) {
            painter.setPen(QColor(220, 230, 240, selected ? 240 : 150));
            painter.setFont(QFont(QStringLiteral("Segoe UI"), 9, selected ? QFont::DemiBold : QFont::Normal));
            painter.drawText(QPointF(labelX, labelY), star.text);
            labelY += 14.0;
        }

        painter.setPen(QColor(160, 190, 210, selected ? 180 : 100));
        painter.setFont(QFont(QStringLiteral("Consolas"), 8));
        painter.drawText(QPointF(labelX, labelY),
                         QStringLiteral("(%1%, %2%, %3%)")
                             .arg(nx, 0, 'f', 1)
                             .arg(ny, 0, 'f', 1)
                             .arg(nz, 0, 'f', 1));
    }
}

// ── Mouse interaction ────────────────────────────────────────────────────────

void StarMapWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const int hit = hitTest(event->pos());
    if (hit >= 0) {
        setSelectedIndex(hit);
        emit starSelected(hit);
        m_dragging = true;
        m_dragStart = event->pos();
    } else {
        setSelectedIndex(-1);
        emit starSelected(-1);
    }
}

void StarMapWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!m_dragging || m_selectedIndex < 0) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPointF delta = event->pos() - m_dragStart;
    m_dragStart = event->pos();

    const QVector3D worldDelta = unprojectDelta(delta, 0.0f);
    QVector3D newPos = m_stars[m_selectedIndex].position + worldDelta;
    newPos.setX(std::clamp(newPos.x(), kSpaceMinX, kSpaceMaxX));
    newPos.setY(std::clamp(newPos.y(), kSpaceMinY, kSpaceMaxY));

    m_stars[m_selectedIndex].position = newPos;
    emit starMoved(m_selectedIndex, newPos);
    update();
}

void StarMapWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
        m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

void StarMapWidget::wheelEvent(QWheelEvent *event) {
    if (m_selectedIndex < 0) {
        QWidget::wheelEvent(event);
        return;
    }

    const float dz = (event->angleDelta().y() > 0) ? kWheelStep : -kWheelStep;
    QVector3D &pos = m_stars[m_selectedIndex].position;
    pos.setZ(std::clamp(pos.z() + dz, kEditorMinZ, kSpaceMaxZ));

    emit starMoved(m_selectedIndex, pos);
    update();
}

// ── Keyboard interaction ─────────────────────────────────────────────────────

void StarMapWidget::keyPressEvent(QKeyEvent *event) {
    if (m_selectedIndex < 0) {
        QWidget::keyPressEvent(event);
        return;
    }

    QVector3D &pos = m_stars[m_selectedIndex].position;
    bool moved = true;

    switch (event->key()) {
    case Qt::Key_Left:  pos.setX(std::clamp(pos.x() - kMoveStep, kSpaceMinX, kSpaceMaxX)); break;
    case Qt::Key_Right: pos.setX(std::clamp(pos.x() + kMoveStep, kSpaceMinX, kSpaceMaxX)); break;
    case Qt::Key_Up:    pos.setY(std::clamp(pos.y() + kMoveStep, kSpaceMinY, kSpaceMaxY)); break;
    case Qt::Key_Down:  pos.setY(std::clamp(pos.y() - kMoveStep, kSpaceMinY, kSpaceMaxY)); break;
    case Qt::Key_W:     pos.setZ(std::clamp(pos.z() + kMoveStep, kEditorMinZ, kSpaceMaxZ)); break;
    case Qt::Key_S:     pos.setZ(std::clamp(pos.z() - kMoveStep, kEditorMinZ, kSpaceMaxZ)); break;
    default:
        moved = false;
        QWidget::keyPressEvent(event);
        break;
    }

    if (moved) {
        emit starMoved(m_selectedIndex, pos);
        update();
    }
}
