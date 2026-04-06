#include "CrawlWindow.h"
#include "TextIO.h"

#include <QCloseEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QPaintEvent>
#include <QPainter>
#include <QPolygonF>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QTransform>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace {

constexpr qreal kPi = 3.14159265358979323846;
constexpr float kSpaceMinX = -640.0f;
constexpr float kSpaceMaxX =  640.0f;
constexpr float kSpaceMinY = -520.0f;
constexpr float kSpaceMaxY =  440.0f;
constexpr float kSpaceMinZ = -1000.0f;
constexpr float kSpaceMaxZ = 2360.0f;

// Spaceflight physics
constexpr float kShipFriction     = 0.88f;
constexpr float kAccelLateral     = 1.8f;
constexpr float kAccelVertical    = 1.5f;
constexpr float kAccelForward     = 2.8f;
constexpr float kMaxLateralSpeed  = 7.5f;
constexpr float kMaxForwardSpeed  = 11.0f;
constexpr float kGoalStopOffsetY  = 15.0f;
constexpr float kGoalApproachZ    = -150.0f;
constexpr float kGoalSpeedDivisor = 5.5f;
constexpr int   kMinLegDuration   = 120;
constexpr qreal kGoalProximity    = 240.0;

// Starfield
constexpr int   kStarfieldCount   = 220;
constexpr int   kSpaceStarCount   = 1200;

// Spaceflight fade
constexpr qreal kSpaceflightFadeFrames = 36.0;
constexpr qreal kCrawlOverlayFadeRate  = 1.0 / 48.0;

qreal clamp01(const qreal value) {
    return std::clamp(value, 0.0, 1.0);
}

qreal easeInOutCubic(const qreal t) {
    const qreal x = clamp01(t);
    return (x < 0.5) ? 4.0 * x * x * x : 1.0 - std::pow(-2.0 * x + 2.0, 3.0) / 2.0;
}

qreal easeInOutSine(const qreal t) {
    const qreal x = clamp01(t);
    return -(std::cos(kPi * x) - 1.0) / 2.0;
}

qreal easeOutCubic(const qreal t) {
    const qreal x = 1.0 - clamp01(t);
    return 1.0 - x * x * x;
}

qreal easeOutQuad(const qreal t) {
    const qreal x = clamp01(t);
    return 1.0 - (1.0 - x) * (1.0 - x);
}

qreal easeInQuad(const qreal t) {
    const qreal x = clamp01(t);
    return x * x;
}

} // namespace

// ── Construction ──────────────────────────────────────────────────────────────

CrawlWindow::CrawlWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("StarWarsText - Show"));
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAutoFillBackground(false);
    setMinimumSize(960, 540);
    regenerateStars();

    m_animationTimer.setInterval(16);
    connect(&m_animationTimer, &QTimer::timeout, this, [this]() {
        switch (m_phase) {
        case Phase::Intro:      tickIntro();      break;
        case Phase::Logo:       tickLogo();       break;
        case Phase::Crawl:      tickCrawl();      break;
        case Phase::Spaceflight:tickSpaceflight();break;
        case Phase::ThreeStars: tickThreeStars(); break;
        case Phase::Ending:     tickEnding();     break;
        }
        update();
    });

    m_resizeDebounce.setSingleShot(true);
    m_resizeDebounce.setInterval(100);
    connect(&m_resizeDebounce, &QTimer::timeout, this, [this]() {
        if (m_phase == Phase::Crawl || m_phase == Phase::ThreeStars)
            rebuildLines();
    });

}

// ── Public interface ──────────────────────────────────────────────────────────

void CrawlWindow::setContent(const CrawlContent &content) {
    m_content = content;
}

void CrawlWindow::setGoalStars(const std::vector<StarDefinition> &stars) {
    m_goalStars = stars;
    m_starMessages.clear();
    m_starMessages.reserve(m_goalStars.size());

    for (const StarDefinition &star : m_goalStars) {
        const qreal nx = clamp01((star.position.x() - kSpaceMinX) / (kSpaceMaxX - kSpaceMinX));
        const qreal ny = clamp01((star.position.y() - kSpaceMinY) / (kSpaceMaxY - kSpaceMinY));
        m_starMessages.push_back({
            star.text,
            QPointF(nx, ny),
            star.coreColor,
            star.glowColor
        });
    }
}

void CrawlWindow::setShowMode(const ShowMode mode) {
    m_showMode = mode;
}

void CrawlWindow::openShowWindow(const bool fullscreen) {
    if (!m_hasInitializedWindowGeometry) {
        resize(1280, 720);
        m_hasInitializedWindowGeometry = true;
    }
    showNormal();
    if (fullscreen)
        setWindowState(windowState() | Qt::WindowFullScreen);
    raise();
    activateWindow();
    m_elapsedTimer.start();
    if (m_showMode == ShowMode::Live)
        initializeSpaceflightScene();
    transitionTo(m_showMode == ShowMode::Live ? Phase::Intro : Phase::Spaceflight);
    m_animationTimer.start();
    update();
}

// ── Qt overrides ──────────────────────────────────────────────────────────────

void CrawlWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const bool live3dBackground =
        m_showMode == ShowMode::Live &&
        (m_phase == Phase::Intro || m_phase == Phase::Logo || m_phase == Phase::Crawl);

    if (live3dBackground)
        paintSpaceScene(painter, false, false);
    else
        paintStarfield(painter);

    switch (m_phase) {
    case Phase::Intro:       paintIntro(painter);      break;
    case Phase::Logo:        paintLogo(painter);       break;
    case Phase::Crawl:       paintCrawl(painter);      break;
    case Phase::Spaceflight: paintSpaceflight(painter);break;
    case Phase::ThreeStars:  paintThreeStars(painter); break;
    case Phase::Ending:      paintEnding(painter);     break;
    }

    paintHUD(painter);
}

void CrawlWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    m_resizeDebounce.start();
}

void CrawlWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    if (event->key() == Qt::Key_F11) {
        setWindowState(windowState() ^ Qt::WindowFullScreen);
        return;
    }
    if (event->key() == Qt::Key_Space) {
        advanceToNextPhase();
        return;
    }
    if (m_phase == Phase::Spaceflight) {
        switch (event->key()) {
        case Qt::Key_Left:  m_input.left = true;     return;
        case Qt::Key_Right: m_input.right = true;    return;
        case Qt::Key_Up:    m_input.up = true;       return;
        case Qt::Key_Down:  m_input.down = true;     return;
        case Qt::Key_W:     m_input.forward = true;  return;
        case Qt::Key_S:     m_input.backward = true; return;
        default: break;
        }
    }
    QWidget::keyPressEvent(event);
}

void CrawlWindow::keyReleaseEvent(QKeyEvent *event) {
    if (m_phase == Phase::Spaceflight) {
        switch (event->key()) {
        case Qt::Key_Left:  m_input.left = false;     return;
        case Qt::Key_Right: m_input.right = false;    return;
        case Qt::Key_Up:    m_input.up = false;       return;
        case Qt::Key_Down:  m_input.down = false;     return;
        case Qt::Key_W:     m_input.forward = false;  return;
        case Qt::Key_S:     m_input.backward = false; return;
        default: break;
        }
    }
    QWidget::keyReleaseEvent(event);
}

void CrawlWindow::closeEvent(QCloseEvent *event) {
    m_animationTimer.stop();
    emit closed();
    QWidget::closeEvent(event);
}

// ── Phase management ──────────────────────────────────────────────────────────

void CrawlWindow::transitionTo(Phase phase) {
    m_phase = phase;
    switch (phase) {
    case Phase::Intro:
        m_introTick = 0;
        break;
    case Phase::Logo:
        m_logoTick = 0;
        break;
    case Phase::Crawl:
        m_crawlOffset = 0.0;
        m_cameraTilt  = 0.0;
        m_starDriftY  = 0.0;
        rebuildLines();
        break;
    case Phase::Spaceflight:
        initializeSpaceflightScene();
        m_spaceflightTick = 0;
        m_spaceflightFade = 0.0;
        m_liveCrawlOverlayOpacity = (m_showMode == ShowMode::Live) ? 1.0 : 0.0;
        m_liveFlight = {};
        m_liveFlight.legStart = m_shipPosition;
        m_liveFlight.legEnd = m_shipPosition;
        m_input = {};
        break;
    case Phase::ThreeStars:
        m_threeStarsStage          = ThreeStarsStage::Entry;
        m_threeStarsTick           = 0;
        m_currentMessageIndex      = 0;
        m_previousMessageIndex     = 0;
        m_threeStarsEntryFade      = 0.0;
        m_threeStarsTravel         = 0.0;
        m_threeStarsMessageOpacity = 0.0;
        m_threeStarsMessageScale   = 0.97;
        m_cameraTilt               = 1.0;
        break;
    case Phase::Ending:
        m_endingTick        = 0;
        m_endingApproach    = 0.0;
        m_endingTextOpacity = 0.0;
        break;
    }
}

// ── Per-phase tick ────────────────────────────────────────────────────────────

void CrawlWindow::advanceToNextPhase() {
    switch (m_phase) {
    case Phase::Intro:
        transitionTo(Phase::Logo);
        break;
    case Phase::Logo:
        transitionTo(Phase::Crawl);
        break;
    case Phase::Crawl:
        transitionTo(Phase::Spaceflight);
        break;
    case Phase::Spaceflight:
        if (m_showMode == ShowMode::Live && m_liveFlight.targetReached) {
            if (m_liveFlight.autoTargetIndex + 1 < static_cast<int>(m_goalStars.size())) {
                ++m_liveFlight.autoTargetIndex;
                m_liveFlight.targetReached = false;
                m_liveFlight.legStart = m_shipPosition;
                m_liveFlight.legEnd = m_goalStars[m_liveFlight.autoTargetIndex].position + QVector3D(0.0f, kGoalStopOffsetY, kGoalApproachZ);
                m_liveFlight.legTick = 0;
                const float distance = (m_liveFlight.legEnd - m_liveFlight.legStart).length();
                m_liveFlight.legDuration = std::max(kMinLegDuration, static_cast<int>(distance / kGoalSpeedDivisor));
                m_liveFlight.legActive = true;
            } else {
                transitionTo(Phase::Ending);
            }
        }
        break;
    case Phase::ThreeStars:
        if (m_threeStarsStage == ThreeStarsStage::Hold) {
            m_previousMessageIndex = m_currentMessageIndex;
            m_threeStarsStage = ThreeStarsStage::Transition;
            m_threeStarsTick  = 0;
        }
        break;
    case Phase::Ending:
        break;
    }
}

static constexpr int kIntroFadeInTicks  = 75;   // ~1.2 s
static constexpr int kIntroHoldTicks    = 150;  // ~2.4 s
static constexpr int kIntroFadeOutTicks = 75;   // ~1.2 s
static constexpr int kIntroTotalTicks   = kIntroFadeInTicks + kIntroHoldTicks + kIntroFadeOutTicks;

void CrawlWindow::tickIntro() {
    ++m_introTick;
    if (m_introTick >= kIntroTotalTicks)
        transitionTo(Phase::Logo);
}

static constexpr int kLogoTotalTicks = 420; // ~6.7 s

void CrawlWindow::tickLogo() {
    ++m_logoTick;
    if (m_logoTick >= kLogoTotalTicks)
        transitionTo(Phase::Crawl);
}

void CrawlWindow::tickCrawl() {
    m_crawlOffset += scrollStep();
    const bool liveTimeoutReached = (m_showMode == ShowMode::Live && m_elapsedTimer.elapsed() >= 45'000);
    if (liveTimeoutReached || m_crawlOffset >= m_crawlTriggerOffset)
        transitionTo(Phase::Spaceflight);
}

void CrawlWindow::tickSpaceflight() {
    ++m_spaceflightTick;
    m_spaceflightFade = std::min(1.0, static_cast<qreal>(m_spaceflightTick) / kSpaceflightFadeFrames);

    if (m_showMode == ShowMode::Live) {
        if (!m_liveFlight.legActive && !m_liveFlight.targetReached && m_liveFlight.autoTargetIndex < static_cast<int>(m_goalStars.size())) {
            m_liveFlight.legStart = m_shipPosition;
            m_liveFlight.legEnd = m_goalStars[m_liveFlight.autoTargetIndex].position + QVector3D(0.0f, kGoalStopOffsetY, kGoalApproachZ);
            m_liveFlight.legTick = 0;
            const float distance = (m_liveFlight.legEnd - m_liveFlight.legStart).length();
            m_liveFlight.legDuration = std::max(kMinLegDuration, static_cast<int>(distance / kGoalSpeedDivisor));
            m_liveFlight.legActive = true;
        }

        if (m_liveFlight.legActive) {
            ++m_liveFlight.legTick;
            const qreal t = clamp01(static_cast<qreal>(m_liveFlight.legTick) / std::max(1, m_liveFlight.legDuration));
            const qreal eased = easeInOutSine(t);
            m_shipPosition = m_liveFlight.legStart + (m_liveFlight.legEnd - m_liveFlight.legStart) * eased;
            m_shipVelocity = (m_liveFlight.legEnd - m_liveFlight.legStart) * static_cast<float>(1.0 / std::max(1, m_liveFlight.legDuration));
            m_liveCrawlOverlayOpacity = std::max(0.0, m_liveCrawlOverlayOpacity - kCrawlOverlayFadeRate);

            if (m_liveFlight.legTick >= m_liveFlight.legDuration) {
                m_shipPosition = m_liveFlight.legEnd;
                m_shipVelocity = QVector3D();
                m_liveFlight.legActive = false;
                m_liveFlight.targetReached = true;
            }
        }

        recycleSpaceStars();
        return;
    }

    QVector3D desired;
    if (m_input.left)     desired.setX(desired.x() - 1.0f);
    if (m_input.right)    desired.setX(desired.x() + 1.0f);
    if (m_input.up)       desired.setY(desired.y() - 1.0f);
    if (m_input.down)     desired.setY(desired.y() + 1.0f);
    if (m_input.forward)  desired.setZ(desired.z() + 1.0f);
    if (m_input.backward) desired.setZ(desired.z() - 1.0f);

    if (!desired.isNull())
        desired.normalize();

    QVector3D acceleration(desired.x() * kAccelLateral, desired.y() * kAccelVertical, desired.z() * kAccelForward);
    m_shipVelocity *= kShipFriction;
    m_shipVelocity += acceleration;

    m_shipVelocity.setX(std::clamp(m_shipVelocity.x(), -kMaxLateralSpeed, kMaxLateralSpeed));
    m_shipVelocity.setY(std::clamp(m_shipVelocity.y(), -kMaxLateralSpeed, kMaxLateralSpeed));
    m_shipVelocity.setZ(std::clamp(m_shipVelocity.z(), -kMaxForwardSpeed, kMaxForwardSpeed));
    m_shipPosition += m_shipVelocity;

    if (m_shipPosition.x() <= kSpaceMinX || m_shipPosition.x() >= kSpaceMaxX)
        m_shipVelocity.setX(0.0f);
    if (m_shipPosition.y() <= kSpaceMinY || m_shipPosition.y() >= kSpaceMaxY)
        m_shipVelocity.setY(0.0f);
    if (m_shipPosition.z() <= kSpaceMinZ || m_shipPosition.z() >= kSpaceMaxZ)
        m_shipVelocity.setZ(0.0f);

    m_shipPosition.setX(std::clamp(m_shipPosition.x(), kSpaceMinX, kSpaceMaxX));
    m_shipPosition.setY(std::clamp(m_shipPosition.y(), kSpaceMinY, kSpaceMaxY));
    m_shipPosition.setZ(std::clamp(m_shipPosition.z(), kSpaceMinZ, kSpaceMaxZ));

    recycleSpaceStars();
}

static constexpr int kThreeStarsEntryTicks      = 30;
static constexpr int kThreeStarsAcquireTicks    = 170;
static constexpr int kThreeStarsTravelTicks     = 118;
static constexpr int kThreeStarsRevealTicks     = 34;
static constexpr int kThreeStarsTransitionTicks = 120;
static constexpr int kThreeStarsHoldTicks       = 150;

void CrawlWindow::tickThreeStars() {
    ++m_threeStarsTick;

    switch (m_threeStarsStage) {
    case ThreeStarsStage::Entry:
        m_threeStarsEntryFade = easeOutQuad(static_cast<qreal>(m_threeStarsTick) / kThreeStarsEntryTicks);
        if (m_threeStarsTick >= kThreeStarsEntryTicks) {
            m_previousMessageIndex = 0;
            m_threeStarsStage      = ThreeStarsStage::Acquire;
            m_threeStarsTick  = 0;
            m_threeStarsEntryFade = 1.0;
        }
        break;

    case ThreeStarsStage::Acquire:
        if (m_threeStarsTick >= kThreeStarsAcquireTicks) {
            m_threeStarsStage = ThreeStarsStage::Travel;
            m_threeStarsTick  = 0;
        }
        break;

    case ThreeStarsStage::Travel:
        m_threeStarsTravel = easeInOutCubic(static_cast<qreal>(m_threeStarsTick) / kThreeStarsTravelTicks);
        if (m_threeStarsTick >= kThreeStarsTravelTicks) {
            m_threeStarsStage  = ThreeStarsStage::Reveal;
            m_threeStarsTick   = 0;
            m_threeStarsTravel = 1.0;
        }
        break;

    case ThreeStarsStage::Reveal:
        m_threeStarsMessageOpacity = easeOutCubic(static_cast<qreal>(m_threeStarsTick) / kThreeStarsRevealTicks);
        m_threeStarsMessageScale   = 0.97 + 0.03 * m_threeStarsMessageOpacity;
        m_threeStarsTravel         = 1.0;
        if (m_threeStarsTick >= kThreeStarsRevealTicks) {
            m_threeStarsStage          = ThreeStarsStage::Hold;
            m_threeStarsTick           = 0;
            m_threeStarsMessageOpacity = 1.0;
            m_threeStarsMessageScale   = 1.0;
            m_threeStarsTravel         = 1.0;
        }
        break;

    case ThreeStarsStage::Hold:
        m_threeStarsTravel = 1.0;
        if (m_showMode == ShowMode::Live && m_threeStarsTick >= kThreeStarsHoldTicks) {
            m_previousMessageIndex = m_currentMessageIndex;
            m_threeStarsStage = ThreeStarsStage::Transition;
            m_threeStarsTick  = 0;
        }
        break;

    case ThreeStarsStage::Transition: {
        const qreal t = static_cast<qreal>(m_threeStarsTick) / kThreeStarsTransitionTicks;
        const qreal eased = easeInOutCubic(t);
        m_threeStarsTravel         = 0.0;
        m_threeStarsMessageOpacity = 1.0 - easeOutQuad(t);
        m_threeStarsMessageScale   = 1.0 + 0.02 * eased;

        if (m_threeStarsTick >= kThreeStarsTransitionTicks) {
            ++m_currentMessageIndex;
            if (m_currentMessageIndex >= static_cast<int>(m_starMessages.size())) {
                transitionTo(Phase::Ending);
                return;
            }

            m_threeStarsStage          = ThreeStarsStage::Acquire;
            m_threeStarsTick           = 0;
            m_threeStarsTravel         = 0.0;
            m_threeStarsMessageOpacity = 0.0;
            m_threeStarsMessageScale   = 0.97;
        }
        break;
    }
    }
}

void CrawlWindow::tickEnding() {
    ++m_endingTick;
}

// ── Per-phase paint ───────────────────────────────────────────────────────────

void CrawlWindow::paintStarfield(QPainter &painter) {
    painter.fillRect(rect(), QColor(0, 0, 0));
    painter.setPen(Qt::NoPen);
    const qreal w = std::max(1, width());
    const qreal h = std::max(1, height());
    const bool threeStarsActive = (m_phase == Phase::ThreeStars && m_currentMessageIndex < static_cast<int>(m_starMessages.size()));
    const QPointF targetPoint = threeStarsActive ? starfieldFocusPoint()
                                                 : QPointF(width() * 0.5, height() * 0.5);
    const qreal travel = threeStarsActive ? m_threeStarsTravel : 0.0;
    const qreal speed  = threeStarsActive ? threeStarsTravelSpeed() : 0.0;
    const qreal phaseTime = m_elapsedTimer.isValid() ? static_cast<qreal>(m_elapsedTimer.elapsed()) / 1000.0 : 0.0;
    QPointF cameraOffset;
    if (threeStarsActive) {
        cameraOffset = threeStarsCameraOffset();
    }

    for (const Star &star : m_stars) {
        qreal x = star.position.x() * w + cameraOffset.x();
        qreal y = star.position.y() * h - m_starDriftY + cameraOffset.y();
        x -= std::floor(x / w) * w;
        y -= std::floor(y / h) * h;
        QPointF point(x, y);
        qreal radius = star.radius;
        QColor color = star.color;

        if (threeStarsActive) {
            const QPointF delta = point - targetPoint;
            const qreal depthWeight = 0.35 + (1.0 - star.depth) * 1.7;
            const qreal travelPush = travel * (0.16 + 0.92 * easeOutCubic(travel)) * depthWeight;
            point += delta * travelPush;
            radius *= 1.0 + travel * 0.45 + speed * 0.22 * depthWeight;

            const qreal distance = std::hypot(delta.x(), delta.y());
            const qreal emphasis = clamp01(1.0 - distance / std::max<qreal>(220.0, width() * 0.24));
            const qreal twinkle  = 0.55 + 0.45 * std::sin(phaseTime * (0.8 + (1.0 - star.depth) * 1.3) + star.twinklePhase);
            const int alpha = std::clamp(
                static_cast<int>(70 + 95 * twinkle + 75 * emphasis + 35 * speed * depthWeight),
                0,
                255);
            color.setAlpha(alpha);

            const qreal rayStrength = speed * depthWeight;
            if (rayStrength > 0.12) {
                QPointF direction = delta;
                const qreal length = std::hypot(direction.x(), direction.y());
                if (length > 0.001) {
                    direction /= length;
                    const qreal streakLength = (8.0 + 48.0 * rayStrength) * (0.28 + clamp01(distance / std::max<qreal>(1.0, width() * 0.5)));
                    const QPointF start = point - direction * (streakLength * 0.15);
                    const QPointF end   = point + direction * streakLength;

                    QLinearGradient trail(start, end);
                    trail.setColorAt(0.0, QColor(color.red(), color.green(), color.blue(), 0));
                    trail.setColorAt(0.2, QColor(color.red(), color.green(), color.blue(), std::min(255, color.alpha() / 2)));
                    trail.setColorAt(1.0, QColor(color.red(), color.green(), color.blue(), color.alpha()));

                    painter.setPen(QPen(QBrush(trail), std::max(1.0, radius * (0.75 + rayStrength * 0.8)),
                                        Qt::SolidLine, Qt::RoundCap));
                    painter.drawLine(start, end);
                    painter.setPen(Qt::NoPen);
                }
            }
        }

        painter.setBrush(color);
        painter.drawEllipse(point, std::max(0.6, radius), std::max(0.6, radius));
    }
}

void CrawlWindow::paintIntro(QPainter &painter) {
    int alpha;
    if (m_introTick < kIntroFadeInTicks) {
        alpha = (m_introTick * 255) / kIntroFadeInTicks;
    } else if (m_introTick < kIntroFadeInTicks + kIntroHoldTicks) {
        alpha = 255;
    } else {
        const int fadeOutTick = m_introTick - kIntroFadeInTicks - kIntroHoldTicks;
        alpha = 255 - (fadeOutTick * 255) / kIntroFadeOutTicks;
    }
    alpha = std::clamp(alpha, 0, 255);

    const int fontSize = std::max(16, static_cast<int>(height() * 0.042));
    QFont font(QStringLiteral("Arial"), fontSize, QFont::Normal, /*italic=*/true);

    painter.setFont(font);
    painter.setPen(QColor(100, 180, 220, alpha));
    painter.drawText(rect(), Qt::AlignCenter, m_content.intro);
}

void CrawlWindow::paintLogo(QPainter &painter) {
    if (m_content.logo.isEmpty())
        return;

    const qreal t = static_cast<qreal>(m_logoTick) / kLogoTotalTicks;

    // Exponential scale: starts filling the screen, shrinks to nothing
    const qreal scale = 4.0 * std::pow(0.01, t);

    // Fade out in the final 15%
    const int alpha = (t < 0.85)
        ? 255
        : static_cast<int>(255.0 * (1.0 - (t - 0.85) / 0.15));

    const int baseFontSize = std::max(16, static_cast<int>(height() * 0.13));
    QFont font(QStringLiteral("Arial Black"), baseFontSize);
    font.setLetterSpacing(QFont::AbsoluteSpacing, baseFontSize * 0.12);

    const QFontMetricsF metrics(font);
    const QStringList logoLines = m_content.logo.split(QLatin1Char('\n'));
    qreal textW = 0.0;
    for (const QString &line : logoLines)
        textW = std::max(textW, metrics.horizontalAdvance(line));
    const qreal lineH = metrics.height();
    const qreal textH = lineH * logoLines.size();

    painter.save();
    painter.translate(width() * 0.5, height() * 0.5);
    painter.scale(scale, scale);
    painter.setFont(font);
    painter.setPen(QColor(255, 220, 0, std::clamp(alpha, 0, 255)));
    painter.drawText(QRectF(-textW * 0.5, -textH * 0.5, textW, textH),
                     Qt::AlignCenter, m_content.logo);
    painter.restore();
}

void CrawlWindow::paintCrawl(QPainter &painter) {
    if (m_crawlImage.isNull())
        return;

    const QRectF viewport    = crawlViewport();
    // m_cameraTilt (0→1) raises the horizon and pushes the bottom down,
    // simulating the camera rotating downward to look at the floor of space.
    const qreal topScreenY   = viewport.top()    + viewport.height() * (0.14 - m_cameraTilt * 0.24);
    const qreal bottomScreenY= viewport.bottom() + viewport.height() * (0.02 + m_cameraTilt * 0.10);
    const qreal topWidth     = viewport.width()  * 0.08;
    const qreal bottomWidth  = viewport.width()  * 0.92;
    const qreal topShiftX    = viewport.width()  * 0.025;
    const qreal sourceTop    = m_crawlOffset;
    const qreal sourceBottom = m_crawlOffset + m_sourceWindowHeight;

    QPolygonF sourceQuad;
    sourceQuad << QPointF(0.0,                  sourceTop)
               << QPointF(m_crawlImage.width(), sourceTop)
               << QPointF(m_crawlImage.width(), sourceBottom)
               << QPointF(0.0,                  sourceBottom);

    QPolygonF destinationQuad;
    destinationQuad << QPointF(viewport.center().x() - topWidth * 0.5 + topShiftX, topScreenY)
                    << QPointF(viewport.center().x() + topWidth * 0.5 + topShiftX, topScreenY)
                    << QPointF(viewport.center().x() + bottomWidth * 0.5,           bottomScreenY)
                    << QPointF(viewport.center().x() - bottomWidth * 0.5,           bottomScreenY);

    QTransform transform;
    if (QTransform::quadToQuad(sourceQuad, destinationQuad, transform)) {
        painter.setWorldTransform(transform, true);
        painter.drawImage(QPointF(0.0, 0.0), m_crawlImage);
        painter.resetTransform();
    }

    const qreal horizonY = topScreenY;
    QLinearGradient fadeGradient(0.0, horizonY - 30.0, 0.0, viewport.top() + viewport.height() * 0.42);
    fadeGradient.setColorAt(0.0,  QColor(0, 0, 0, 255));
    fadeGradient.setColorAt(0.45, QColor(0, 0, 0, 150));
    fadeGradient.setColorAt(1.0,  QColor(0, 0, 0,   0));
    painter.fillRect(
        QRectF(viewport.left(), viewport.top(), viewport.width(), viewport.height() * 0.45),
        fadeGradient);
}

void CrawlWindow::paintSpaceScene(QPainter &painter, const bool showGoalText, const bool showHudCube,
                                  const qreal crawlOverlayOpacity) {
    painter.fillRect(rect(), QColor(0, 0, 0));
    painter.setPen(Qt::NoPen);
    const qreal phaseTime = m_elapsedTimer.isValid() ? static_cast<qreal>(m_elapsedTimer.elapsed()) / 1000.0 : 0.0;

    for (const SpaceStar &star : m_spaceStars) {
        const QVector3D relative = star.position - m_shipPosition;
        if (relative.z() <= 12.0f)
            continue;

        qreal scale = 0.0;
        const QPointF point = projectSpacePoint(star.position, &scale);
        if (point.x() < -80.0 || point.x() > width() + 80.0 || point.y() < -80.0 || point.y() > height() + 80.0)
            continue;

        QColor color = star.color;
        const qreal twinkle = 0.55 + 0.45 * std::sin(phaseTime * (0.7 + star.radius * 0.25) + star.twinklePhase);
        color.setAlpha(std::clamp(static_cast<int>(70 + 130 * twinkle), 0, 255));

        const qreal radius = std::max(0.18, star.radius * scale * 9.0);
        painter.setBrush(color);
        painter.drawEllipse(point, radius, radius);
    }

    std::vector<int> goalOrder(m_goalStars.size());
    std::iota(goalOrder.begin(), goalOrder.end(), 0);
    std::stable_sort(goalOrder.begin(), goalOrder.end(), [this](const int lhs, const int rhs) {
        const float lhsZ = (m_goalStars[lhs].position - m_shipPosition).z();
        const float rhsZ = (m_goalStars[rhs].position - m_shipPosition).z();
        return lhsZ > rhsZ;
    });

    for (const int index : goalOrder) {
        const StarDefinition &goal = m_goalStars[index];
        const QVector3D relative = goal.position - m_shipPosition;
        if (relative.z() <= 12.0f)
            continue;

        qreal scale = 0.0;
        const QPointF point = projectSpacePoint(goal.position, &scale);
        const qreal distance = relative.length();
        const qreal emphasis = clamp01(1.0 - distance / 1000.0);
        
        // Size scales strictly with perspective, maintaining a minimum dot size for distant stars
        const qreal radius = std::max<qreal>(0.8, scale * goal.radius * 3.5);
        
        // The glow expands significantly as we get closer (simulating lens flare/bloom)
        const qreal glowRadius = radius * (2.0 + emphasis * 8.0);

        QRadialGradient glow(point, glowRadius, point);
        // Create a hot core that falls off to the glow color, then fades out
        glow.setColorAt(0.00, QColor(255, 255, 255, std::clamp(static_cast<int>(120 + emphasis * 135), 0, 255)));
        glow.setColorAt(0.15, QColor(goal.glowColor.red(), goal.glowColor.green(), goal.glowColor.blue(),
                                    std::clamp(static_cast<int>(90 + emphasis * 120), 0, 255)));
        glow.setColorAt(0.45, QColor(goal.glowColor.red(), goal.glowColor.green(), goal.glowColor.blue(),
                                     std::clamp(static_cast<int>(30 + emphasis * 60), 0, 255)));
        glow.setColorAt(1.00, QColor(0, 0, 0, 0));
        
        // Fill only the bounding box of the glow for better performance
        painter.fillRect(QRectF(point.x() - glowRadius, point.y() - glowRadius, glowRadius * 2.0, glowRadius * 2.0), glow);

        painter.setBrush(goal.coreColor);
        painter.drawEllipse(point, radius, radius);
    }

    if (crawlOverlayOpacity > 0.0) {
        painter.setOpacity(crawlOverlayOpacity);
        paintCrawl(painter);
        painter.setOpacity(1.0);
    }

    const int goalIndex = activeGoalStarIndex();
    if (showGoalText && goalIndex >= 0) {
        const StarDefinition &goal = m_goalStars[goalIndex];
        const QRectF rect(width() * 0.18, height() * 0.72, width() * 0.64, height() * 0.14);
        QFont font(QStringLiteral("Arial"), std::max(24, static_cast<int>(height() * 0.055)), QFont::DemiBold);

        painter.setFont(font);
        painter.setPen(QColor(goal.glowColor.red(), goal.glowColor.green(), goal.glowColor.blue(), 120));
        for (const QPointF &d : {QPointF(-2, 0), QPointF(2, 0), QPointF(0, -2), QPointF(0, 2)})
            painter.drawText(rect.translated(d), Qt::AlignCenter | Qt::TextWordWrap, goal.text);

        painter.setPen(QColor(248, 246, 240));
        painter.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, goal.text);
    }

    if (!showHudCube)
        return;

    const QRectF cubeRect(width() - 210.0, 24.0, 170.0, 170.0);
    const QPointF depthOffset(20.0, -20.0);
    const QRectF backRect = cubeRect.translated(depthOffset);
    const QColor cubeColor(120, 255, 150, 150);

    painter.setBrush(QColor(20, 60, 28, 28));
    painter.setPen(QPen(cubeColor, 1.5));
    painter.drawRect(cubeRect);
    painter.drawRect(backRect);
    painter.drawLine(cubeRect.topLeft(), backRect.topLeft());
    painter.drawLine(cubeRect.topRight(), backRect.topRight());
    painter.drawLine(cubeRect.bottomLeft(), backRect.bottomLeft());
    painter.drawLine(cubeRect.bottomRight(), backRect.bottomRight());

    const qreal nx = (m_shipPosition.x() - kSpaceMinX) / (kSpaceMaxX - kSpaceMinX);
    const qreal ny = (m_shipPosition.y() - kSpaceMinY) / (kSpaceMaxY - kSpaceMinY);
    const qreal nz = (m_shipPosition.z() - kSpaceMinZ) / (kSpaceMaxZ - kSpaceMinZ);
    const QPointF mapPoint(
        cubeRect.left() + cubeRect.width() * nx + depthOffset.x() * (1.0 - nz),
        cubeRect.bottom() - cubeRect.height() * ny + depthOffset.y() * (1.0 - nz));

    for (const StarDefinition &goal : m_goalStars) {
        const qreal gx = (goal.position.x() - kSpaceMinX) / (kSpaceMaxX - kSpaceMinX);
        const qreal gy = (goal.position.y() - kSpaceMinY) / (kSpaceMaxY - kSpaceMinY);
        const qreal gz = (goal.position.z() - kSpaceMinZ) / (kSpaceMaxZ - kSpaceMinZ);
        const QPointF goalPoint(
            cubeRect.left() + cubeRect.width() * gx + depthOffset.x() * (1.0 - gz),
            cubeRect.bottom() - cubeRect.height() * gy + depthOffset.y() * (1.0 - gz));
        painter.setBrush(QColor(goal.coreColor.red(), goal.coreColor.green(), goal.coreColor.blue(), 220));
        painter.setPen(QPen(QColor(goal.glowColor.red(), goal.glowColor.green(), goal.glowColor.blue(), 180), 1.0));
        painter.drawEllipse(goalPoint, 3.8, 3.8);
    }

    painter.setBrush(QColor(140, 255, 170, 220));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(mapPoint, 5.0, 5.0);

    painter.setPen(QColor(140, 255, 170, 190));
    painter.setFont(QFont(QStringLiteral("Consolas"), 10));
    painter.drawText(QRectF(cubeRect.left(), cubeRect.bottom() + 8.0, cubeRect.width(), 18.0),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Ship position"));
    painter.drawText(QRectF(cubeRect.left(), cubeRect.bottom() + 26.0, cubeRect.width(), 18.0),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("x:%1  y:%2  z:%3")
                         .arg(static_cast<int>(std::lround(m_shipPosition.x())))
                         .arg(static_cast<int>(std::lround(m_shipPosition.y())))
                         .arg(static_cast<int>(std::lround(m_shipPosition.z()))));
}

void CrawlWindow::paintSpaceflight(QPainter &painter) {
    const qreal crawlOpacity = (m_showMode == ShowMode::Live) ? m_liveCrawlOverlayOpacity : (1.0 - m_spaceflightFade);
    paintSpaceScene(painter, true, m_showMode != ShowMode::Live, crawlOpacity);
}

void CrawlWindow::paintThreeStars(QPainter &painter) {
    if (m_currentMessageIndex >= static_cast<int>(m_starMessages.size()))
        return;

    const StarMessage &message = m_starMessages[std::clamp(m_currentMessageIndex, 0, static_cast<int>(m_starMessages.size()) - 1)];
    const QPointF target = adjustedStarPoint(m_currentMessageIndex);

    if (m_threeStarsStage == ThreeStarsStage::Entry) {
        const qreal crawlOpacity = 1.0 - m_threeStarsEntryFade;
        painter.setOpacity(crawlOpacity);
        paintCrawl(painter);
        painter.setOpacity(1.0);
    }

    const qreal acquisition = (m_threeStarsStage == ThreeStarsStage::Acquire)
        ? easeOutQuad(static_cast<qreal>(m_threeStarsTick) / kThreeStarsAcquireTicks)
        : 1.0;
    const qreal travel = m_threeStarsTravel;
    const auto paintStar = [&](const StarMessage &starMessage, const QPointF &point, const qreal starTravel,
                               const qreal starAcquisition, const qreal alphaScale, const bool neighbors) {
        if (alphaScale <= 0.0)
            return;

        const qreal haloRadius = std::max(width(), height()) * (0.035 + 0.18 * starTravel);
        const qreal coreRadius = 3.0 + 5.0 * starAcquisition + 28.0 * starTravel;
        const int glowAlpha = std::clamp(
            static_cast<int>((90 + 110 * starAcquisition + 35 * starTravel) * alphaScale),
            0,
            255);

        QRadialGradient halo(point, haloRadius, point);
        halo.setColorAt(0.00, QColor(starMessage.glowColor.red(), starMessage.glowColor.green(),
                                     starMessage.glowColor.blue(), glowAlpha));
        halo.setColorAt(0.18, QColor(starMessage.glowColor.red(), starMessage.glowColor.green(),
                                     starMessage.glowColor.blue(), glowAlpha / 2));
        halo.setColorAt(1.00, QColor(0, 0, 0, 0));
        const QRectF haloBounds(point.x() - haloRadius, point.y() - haloRadius,
                                haloRadius * 2.0, haloRadius * 2.0);
        painter.fillRect(haloBounds, halo);

        if (starTravel > 0.05) {
            const qreal vignetteAlpha = std::min(0.55, starTravel * 0.45) * alphaScale;
            painter.fillRect(rect(), QColor(0, 0, 0, static_cast<int>(255 * vignetteAlpha)));
            painter.fillRect(haloBounds, halo);
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(starMessage.coreColor.red(), starMessage.coreColor.green(),
                                starMessage.coreColor.blue(),
                                std::clamp(static_cast<int>(255 * alphaScale), 0, 255)));
        painter.drawEllipse(point, coreRadius, coreRadius);

        if (neighbors) {
            painter.setBrush(QColor(255, 210, 170, std::clamp(static_cast<int>(170 * alphaScale), 0, 255)));
            const qreal orbitRadius = std::max(width(), height()) * (0.04 + 0.04 * starTravel);
            for (int i = 0; i < 4; ++i) {
                const qreal angle = 0.45 + i * 0.7;
                const QPointF neighbor(
                    point.x() + std::cos(angle * kPi) * orbitRadius,
                    point.y() + std::sin(angle * kPi) * orbitRadius * 0.55);
                painter.drawEllipse(neighbor, 1.8 + starTravel * 2.2, 1.8 + starTravel * 2.2);
            }
        }
    };

    if (m_threeStarsStage == ThreeStarsStage::Transition) {
        const qreal t = easeInOutCubic(static_cast<qreal>(m_threeStarsTick) / kThreeStarsTransitionTicks);
        const qreal fadeOut = 1.0 - easeOutCubic(std::min(1.0, t * 1.6));
        const int nextIndex = (m_currentMessageIndex + 1) % static_cast<int>(m_starMessages.size());
        const StarMessage &nextMessage = m_starMessages[nextIndex];
        const QPointF nextTarget = adjustedStarPoint(nextIndex);
        const qreal nextAcquire = easeOutQuad(t);

        paintStar(message, target, 1.0, 1.0, fadeOut, m_currentMessageIndex == 2);
        paintStar(nextMessage, nextTarget, 0.0, nextAcquire, 0.10 + 0.90 * t, nextIndex == 2);
    } else {
        paintStar(message, target, travel, acquisition, 1.0, m_currentMessageIndex == 2);
    }

    if (m_threeStarsMessageOpacity > 0.0 && !message.text.trimmed().isEmpty()) {
        const QRectF rect = messageRect();
        const int fontSize = std::max(24, static_cast<int>(height() * 0.06));
        QFont font(QStringLiteral("Arial"), fontSize, QFont::DemiBold);

        painter.save();
        painter.setOpacity(m_threeStarsMessageOpacity);
        painter.translate(rect.center());
        painter.scale(m_threeStarsMessageScale, m_threeStarsMessageScale);
        painter.translate(-rect.center());
        painter.setFont(font);

        painter.setPen(QColor(message.glowColor.red(), message.glowColor.green(), message.glowColor.blue(),
                               static_cast<int>(110 * m_threeStarsMessageOpacity)));
        for (const QPointF &d : {QPointF(-2, 0), QPointF(2, 0), QPointF(0, -2), QPointF(0, 2)})
            painter.drawText(rect.translated(d), Qt::AlignCenter | Qt::TextWordWrap, message.text);

        painter.setPen(QColor(248, 246, 240, static_cast<int>(255 * m_threeStarsMessageOpacity)));
        painter.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, message.text);
        painter.restore();
    }
}

void CrawlWindow::paintEnding(QPainter &painter) {
    const int w = std::max<int>(1, width());
    const int h = std::max<int>(1, height());
    const QPointF center(static_cast<qreal>(w) * 0.5, static_cast<qreal>(h) * 0.5);

    painter.fillRect(rect(), QColor(0, 0, 0));

    // Hyperspace logic (0 to 300 ticks = 5 seconds)
    qreal speed = 0.0;
    if (m_endingTick <= 300) {
        if (m_endingTick < 60) {
            speed = easeInQuad(static_cast<qreal>(m_endingTick) / 60.0);
        } else if (m_endingTick < 240) {
            speed = 1.0;
        } else {
            speed = 1.0 - easeOutQuad(static_cast<qreal>(m_endingTick - 240) / 60.0);
        }
        
        for (const Star &star : m_stars) {
            qreal dx = star.position.x() * w - center.x();
            qreal dy = star.position.y() * h - center.y();
            const qreal dist = std::hypot(dx, dy);
            if (dist < 1.0) continue;
            
            qreal move = dist * speed * 4.0;
            QPointF currentPos = center + QPointF(dx, dy) * (1.0 + move);
            QPointF startPos = currentPos - QPointF(dx, dy) * (speed * 1.5);
            
            QColor c = star.color;
            qreal starAlpha = 1.0;
            if (m_endingTick > 240) {
                starAlpha = 1.0 - static_cast<qreal>(m_endingTick - 240) / 60.0;
            }
            c.setAlpha(std::clamp(static_cast<int>(star.color.alpha() * starAlpha), 0, 255));
            
            painter.setPen(QPen(c, std::max<qreal>(1.0, star.radius), Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(startPos, currentPos);
        }
    }

    // Logo + Summary Reveal (m_endingTick > 300)
    if (m_endingTick > 300) {
        const qreal reveal = clamp01(static_cast<qreal>(m_endingTick - 300) / 90.0);
        const qreal logoAlpha = easeOutCubic(reveal);

        QColor orangeCore(255, 255, 255, static_cast<int>(255 * logoAlpha));
        QColor orangeGlow(231, 122, 35, static_cast<int>(255 * logoAlpha));
        
        QColor whiteCore(255, 255, 255, static_cast<int>(255 * logoAlpha));
        QColor whiteGlow(200, 200, 200, static_cast<int>(200 * logoAlpha));
        
        QColor cyanCore(255, 255, 255, static_cast<int>(255 * logoAlpha));
        QColor cyanGlow(16, 163, 202, static_cast<int>(255 * logoAlpha));

        // Shifted right to be more centered
        QPointF node1(w * 0.30, h * 0.65);
        QPointF node2(w * 0.43, h * 0.35);
        QPointF node3(w * 0.56, h * 0.65);
        QPointF node4(w * 0.69, h * 0.35);

        const qreal pathWidth = h * 0.05;
        const qreal nodeRadius = h * 0.07;
        const qreal glowRadius = nodeRadius * 2.5;

        if (logoAlpha > 0.0) {
            // Draw paths with sequential reveal
            qreal p1 = clamp01(logoAlpha * 3.0);
            QLinearGradient grad1(node1, node2);
            grad1.setColorAt(0, orangeGlow);
            grad1.setColorAt(1, whiteGlow);
            painter.setPen(QPen(QBrush(grad1), pathWidth, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(node1, node1 + (node2 - node1) * p1);

            if (logoAlpha > 0.33) {
                qreal p2 = clamp01((logoAlpha - 0.33) * 3.0);
                painter.setPen(QPen(whiteGlow, pathWidth, Qt::SolidLine, Qt::RoundCap));
                painter.drawLine(node2, node2 + (node3 - node2) * p2);
            }

            if (logoAlpha > 0.66) {
                qreal p3 = clamp01((logoAlpha - 0.66) * 3.0);
                QLinearGradient grad2(node3, node4);
                grad2.setColorAt(0, whiteGlow);
                grad2.setColorAt(1, cyanGlow);
                painter.setPen(QPen(QBrush(grad2), pathWidth, Qt::SolidLine, Qt::RoundCap));
                painter.drawLine(node3, node3 + (node4 - node3) * p3);
            }

            // Draw Nodes as glowing stars
            auto drawStar = [&](const QPointF &pos, const QColor &core, const QColor &glow, bool isRing) {
                QRadialGradient grad(pos, glowRadius, pos);
                grad.setColorAt(0.00, core);
                grad.setColorAt(0.20, QColor(glow.red(), glow.green(), glow.blue(), static_cast<int>(200 * logoAlpha)));
                grad.setColorAt(1.00, QColor(0, 0, 0, 0));
                painter.fillRect(QRectF(pos.x() - glowRadius, pos.y() - glowRadius, glowRadius * 2, glowRadius * 2), grad);

                if (isRing) {
                    painter.setPen(QPen(core, h * 0.03 * logoAlpha));
                    painter.setBrush(QColor(0, 0, 0));
                    painter.drawEllipse(pos, (nodeRadius - h * 0.01) * logoAlpha, (nodeRadius - h * 0.01) * logoAlpha);
                } else {
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(core);
                    painter.drawEllipse(pos, nodeRadius * logoAlpha * 0.6, nodeRadius * logoAlpha * 0.6);
                }
            };

            drawStar(node1, orangeCore, orangeGlow, false);
            drawStar(node2, whiteCore, whiteGlow, false);
            drawStar(node3, whiteCore, whiteGlow, false);
            drawStar(node4, cyanCore, cyanGlow, true);
        }

        // Text Reveal (m_endingTick > 390)
        if (m_endingTick > 390) {
            const qreal textOpacity = easeOutCubic(clamp01(static_cast<qreal>(m_endingTick - 390) / 60.0));
            painter.setOpacity(textOpacity);
            
            QFont bodyFont(QStringLiteral("Segoe UI"), std::max<int>(16, static_cast<int>(h * 0.025)), QFont::DemiBold);
            painter.setFont(bodyFont);
            painter.setPen(QColor(220, 230, 240));
            
            auto drawLabel = [&](int index, const QPointF &nodePos, bool below) {
                if (index >= static_cast<int>(m_starMessages.size())) return;
                
                const qreal offset = nodeRadius * 2.0;
                QRectF textRect;
                if (below) {
                    textRect = QRectF(nodePos.x() - w * 0.1, nodePos.y() + offset, w * 0.2, h * 0.2);
                    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, m_starMessages[index].text);
                } else {
                    textRect = QRectF(nodePos.x() - w * 0.1, nodePos.y() - offset - h * 0.2, w * 0.2, h * 0.2);
                    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWordWrap, m_starMessages[index].text);
                }
            };

            drawLabel(0, node1, true);  // Below Node 1
            drawLabel(1, node2, false); // Above Node 2
            drawLabel(2, node3, true);  // Below Node 3
            drawLabel(3, node4, false); // Above Node 4
            
            painter.setOpacity(1.0);
        }
    }
}

void CrawlWindow::paintHUD(QPainter &painter) {
    const qint64 ms   = m_elapsedTimer.isValid() ? m_elapsedTimer.elapsed() : 0;
    const int    secs = static_cast<int>(ms / 1000);
    const QString timeStr = QStringLiteral("%1:%2")
        .arg(secs / 60, 2, 10, QLatin1Char('0'))
        .arg(secs % 60, 2, 10, QLatin1Char('0'));

    const QFont  hudFont(QStringLiteral("Consolas"), 11);
    const QRectF hudRect(20.0, height() - 36.0, width() - 40.0, 20.0);

    painter.setFont(hudFont);
    painter.setPen(QColor(180, 180, 180));
    QString hint = QStringLiteral("Esc to editor   F11 fullscreen   Space \u2192 next scene");
    if (m_phase == Phase::Spaceflight && m_showMode == ShowMode::Live && m_liveFlight.targetReached) {
        hint = QStringLiteral("Esc to editor   F11 fullscreen   Space \u2192 next star");
    } else if (m_phase == Phase::Spaceflight) {
        hint = QStringLiteral("Arrows move   W forward   S backward   Esc editor   F11 fullscreen");
    } else if (m_phase == Phase::ThreeStars && m_threeStarsStage == ThreeStarsStage::Hold) {
        hint = QStringLiteral("Esc to editor   F11 fullscreen   Space \u2192 next star");
    }
    painter.drawText(hudRect, Qt::AlignLeft | Qt::AlignVCenter, hint);
    painter.drawText(hudRect, Qt::AlignRight | Qt::AlignVCenter, timeStr);
}

// ── Viewport / scroll ─────────────────────────────────────────────────────────

QRectF CrawlWindow::crawlViewport() const {
    const qreal marginX = width()  * 0.08;
    const qreal marginY = height() * 0.06;
    const qreal availableWidth  = std::max(320.0, width()  - marginX * 2.0);
    const qreal availableHeight = std::max(240.0, height() - marginY * 2.0);
    const qreal targetAspect = 16.0 / 9.0;

    qreal vpWidth  = availableWidth;
    qreal vpHeight = vpWidth / targetAspect;
    if (vpHeight > availableHeight) {
        vpHeight = availableHeight;
        vpWidth  = vpHeight * targetAspect;
    }

    return QRectF((width()  - vpWidth)  * 0.5,
                  (height() - vpHeight) * 0.5,
                  vpWidth, vpHeight);
}

qreal CrawlWindow::scrollStep() const {
    const qreal h = crawlViewport().height();
    qreal normalized;
    if (h <= 900.0)
        normalized = std::clamp((h - 720.0) / 700.0, 0.3, 0.55);
    else
        normalized = std::clamp(0.55 + (h - 900.0) / 260.0 * 0.45, 0.55, 1.0);
    return 1.35 * normalized;
}

// ── Starfield ─────────────────────────────────────────────────────────────────

void CrawlWindow::regenerateStars() {
    m_stars.clear();
    constexpr int kStarCount = kStarfieldCount;
    auto *random = QRandomGenerator::global();

    for (int i = 0; i < kStarCount; ++i) {
        Star star;
        star.position = QPointF(random->bounded(10000) / 10000.0,
                                random->bounded(10000) / 10000.0);
        star.depth    = 0.08 + random->bounded(920) / 1000.0;
        star.radius   = 0.45 + (1.0 - star.depth) * 2.2 + random->bounded(35) / 100.0;
        star.twinklePhase = random->bounded(628) / 100.0;
        const int brightness = 170 + random->bounded(86);
        const int blueTint   = 200 + random->bounded(56);
        star.color = QColor(brightness, brightness, blueTint, 220);
        m_stars.push_back(star);
    }
}

// ── Crawl rendering ───────────────────────────────────────────────────────────

void CrawlWindow::rebuildLines() {
    m_renderLines.clear();
    const QRectF viewport = crawlViewport();

    const int titleSize    = std::max(16, static_cast<int>(viewport.height() / 18.0));
    const int subtitleSize = std::max(16, static_cast<int>(viewport.height() / 20.0));
    const int bodySize     = std::max(14, static_cast<int>(viewport.height() / 27.0));

    const QFont titleFont   (QStringLiteral("Arial"),    titleSize,    QFont::Bold);
    const QFont subtitleFont(QStringLiteral("Arial"),    subtitleSize, QFont::DemiBold);
    const QFont bodyFont    (QStringLiteral("Consolas"), bodySize);

    if (!m_content.title.trimmed().isEmpty())
        appendLine(m_content.title, titleFont, LineAlignment::Center, viewport.height() * 0.025);

    if (!m_content.subtitle.trimmed().isEmpty())
        appendLine(m_content.subtitle, subtitleFont, LineAlignment::Center, viewport.height() * 0.018);

    for (const QString &line : m_content.bodyLines) {
        if (line.trimmed().isEmpty())
            appendLine(QString(), bodyFont, LineAlignment::Left, viewport.height() * 0.004);
        else
            appendLine(line, bodyFont, LineAlignment::Left, viewport.height() * 0.0015);
    }

    renderCrawlImage();
}

void CrawlWindow::appendLine(const QString &text, const QFont &font,
                             LineAlignment alignment, qreal spacingAfter) {
    const QFontMetricsF metrics(font);
    RenderLine line;
    line.text         = text;
    line.font         = font;
    line.alignment    = alignment;
    line.height       = std::max(metrics.height() * 1.0, 12.0);
    line.advance      = std::max(metrics.height() * 0.92, 10.0);
    line.spacingAfter = spacingAfter;
    m_renderLines.push_back(line);
}

void CrawlWindow::renderCrawlImage() {
    const QRectF viewport = crawlViewport();
    const QFont bodyFont(QStringLiteral("Consolas"), std::max(14, static_cast<int>(viewport.height() / 27.0)));
    const QFontMetricsF bodyMetrics(bodyFont);
    const qreal bodyColumnWidth = bodyMetrics.horizontalAdvance(QStringLiteral("X")) * 38.0;
    m_sourceWindowHeight = std::max(860.0, viewport.height() * 3.35);
    const qreal entryPadding = m_sourceWindowHeight * 1.0;
    const qreal exitPadding  = m_sourceWindowHeight * 0.45;
    const int imageWidth  = std::max(320, static_cast<int>(std::ceil(bodyColumnWidth + 80.0)));
    const int imageHeight = std::max(240, static_cast<int>(std::ceil(entryPadding + totalContentHeight() + exitPadding)));

    // Trigger outro when the last content line has moved into the upper ~35% of the
    // source window — it looks tiny/illegible there, so it is the right moment to cut.
    m_crawlTriggerOffset = entryPadding + totalContentHeight() - m_sourceWindowHeight * 0.35;

    m_crawlImage = QImage(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    m_crawlImage.fill(Qt::transparent);

    QPainter imagePainter(&m_crawlImage);
    imagePainter.setRenderHint(QPainter::Antialiasing, true);
    imagePainter.setRenderHint(QPainter::TextAntialiasing, true);

    qreal y = entryPadding;
    for (const RenderLine &line : m_renderLines) {
        imagePainter.setFont(line.font);
        const QRectF lineRect(40.0, y, imageWidth - 80.0, line.height);
        const Qt::Alignment hAlign =
            (line.alignment == LineAlignment::Center) ? Qt::AlignHCenter : Qt::AlignLeft;

        // Soft glow pass
        imagePainter.setPen(QColor(255, 215, 90, 48));
        for (const QPointF &d : {QPointF(-1, 0), QPointF(1, 0), QPointF(0, -1), QPointF(0, 1)})
            imagePainter.drawText(lineRect.translated(d), hAlign | Qt::AlignVCenter, line.text);

        // Main text
        imagePainter.setPen(QColor(255, 228, 110));
        imagePainter.drawText(lineRect, hAlign | Qt::AlignVCenter, line.text);

        y += line.advance + line.spacingAfter;
    }
}

qreal CrawlWindow::totalContentHeight() const {
    qreal total = 0.0;
    for (const RenderLine &line : m_renderLines)
        total += line.advance + line.spacingAfter;
    return total;
}

void CrawlWindow::initializeSpaceflightScene() {
    m_shipPosition = QVector3D(0.0f, 0.0f, kSpaceMinZ);
    m_shipVelocity = QVector3D();

    if (m_goalStars.empty())
        setGoalStars(parseStars(loadRawStars()));

    m_spaceStars.clear();
    auto *random = QRandomGenerator::global();

    for (int i = 0; i < kSpaceStarCount; ++i) {
        SpaceStar star;
        star.position = QVector3D(
            static_cast<float>(random->bounded(-2600, 2601)),
            static_cast<float>(random->bounded(-1800, 1801)),
            static_cast<float>(random->bounded(-200, 5200)));
            
        // ~80% of stars are tiny background "dust", 20% are slightly larger
        if (random->bounded(100) < 80) {
            star.radius = 0.04 + random->bounded(10) / 100.0;
        } else {
            star.radius = 0.15 + random->bounded(30) / 100.0;
        }
        
        star.twinklePhase = random->bounded(628) / 100.0;
        const int brightness = 165 + random->bounded(90);
        const int blueTint   = 200 + random->bounded(55);
        star.color = QColor(brightness, brightness, blueTint, 220);
        m_spaceStars.push_back(star);
    }
}

void CrawlWindow::recycleSpaceStars() {
    auto *random = QRandomGenerator::global();
    for (SpaceStar &star : m_spaceStars) {
        QVector3D relative = star.position - m_shipPosition;

        if (relative.z() < 24.0f)
            star.position.setZ(m_shipPosition.z() + 4200.0f + static_cast<float>(random->bounded(320.0)));
        else if (relative.z() > 5200.0f)
            star.position.setZ(m_shipPosition.z() - 120.0f + static_cast<float>(random->bounded(260.0)));

        if (relative.x() < -2800.0f)
            star.position.setX(m_shipPosition.x() + 2700.0f + static_cast<float>(random->bounded(220.0)));
        else if (relative.x() > 2800.0f)
            star.position.setX(m_shipPosition.x() - 2700.0f - static_cast<float>(random->bounded(220.0)));

        if (relative.y() < -2000.0f)
            star.position.setY(m_shipPosition.y() + 1900.0f + static_cast<float>(random->bounded(180.0)));
        else if (relative.y() > 2000.0f)
            star.position.setY(m_shipPosition.y() - 1900.0f - static_cast<float>(random->bounded(180.0)));
    }
}

QPointF CrawlWindow::projectSpacePoint(const QVector3D &worldPoint, qreal *scale) const {
    const QVector3D relative = worldPoint - m_shipPosition;
    const qreal depth = std::max<qreal>(12.0, relative.z());
    const qreal focal = std::min(width(), height()) * 0.95;
    const qreal pointScale = focal / depth;
    if (scale != nullptr)
        *scale = pointScale;
    return QPointF(width() * 0.5 + relative.x() * pointScale,
                   height() * 0.5 + relative.y() * pointScale);
}

int CrawlWindow::activeGoalStarIndex() const {
    int bestIndex = -1;
    qreal bestScore = 1e9;

    for (int i = 0; i < static_cast<int>(m_goalStars.size()); ++i) {
        const StarDefinition &goal = m_goalStars[i];
        const QVector3D relative = goal.position - m_shipPosition;
        if (relative.z() <= 12.0f)
            continue;

        const qreal distance = relative.length();
        const qreal centerBias = std::hypot(relative.x(), relative.y()) * 0.45;
        const qreal score = distance + centerBias;
        if (score < bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    if (bestIndex < 0)
        return -1;

    const QVector3D relative = m_goalStars[bestIndex].position - m_shipPosition;
    return (relative.length() < kGoalProximity) ? bestIndex : -1;
}

QPointF CrawlWindow::targetPointForMessage(const int index) const {
    if (m_starMessages.empty())
        return QPointF(width() * 0.5, height() * 0.5);

    const StarMessage &message = m_starMessages[std::clamp(index, 0, static_cast<int>(m_starMessages.size()) - 1)];
    return QPointF(message.normalizedPosition.x() * width(), message.normalizedPosition.y() * height());
}

QPointF CrawlWindow::starfieldFocusPoint() const {
    if (m_phase == Phase::Ending)
        return QPointF(width() * 0.5, height() * 0.58);
    return adjustedStarPoint(m_currentMessageIndex);
}

QPointF CrawlWindow::threeStarsCameraOffset() const {
    if (m_phase != Phase::ThreeStars || m_starMessages.empty())
        return QPointF();

    const QPointF shift = acquisitionShiftDirection();
    const qreal distance = (std::abs(shift.x()) > 0.0) ? width() * 0.62 : height() * 0.58;
    const QPointF fullOffset = shift * distance;

    if (m_threeStarsStage == ThreeStarsStage::Entry) {
        const qreal t = easeInOutSine(static_cast<qreal>(m_threeStarsTick) / kThreeStarsEntryTicks);
        return QPointF(0.0, -height() * 0.34 * (1.0 - t));
    }

    if (m_threeStarsStage == ThreeStarsStage::Acquire) {
        const qreal t = easeInOutSine(static_cast<qreal>(m_threeStarsTick) / kThreeStarsAcquireTicks);
        return fullOffset * t;
    }

    if (m_currentMessageIndex == 0)
        return QPointF();

    return fullOffset;
}

QPointF CrawlWindow::adjustedStarPoint(const int index) const {
    return targetPointForMessage(index) + threeStarsCameraOffset();
}

QPointF CrawlWindow::acquisitionShiftDirection() const {
    if (m_phase == Phase::Ending)
        return QPointF(1.0, 0.0);

    switch (m_currentMessageIndex) {
    case 0: return QPointF(0.0, -1.0);
    case 1: return QPointF(-1.0, 0.0);
    case 2: return QPointF(0.0, 1.0);
    default: return QPointF();
    }
}

QRectF CrawlWindow::messageRect() const {
    const qreal rectWidth  = width() * 0.66;
    const qreal rectHeight = height() * 0.18;
    return QRectF((width() - rectWidth) * 0.5,
                  height() * 0.69,
                  rectWidth,
                  rectHeight);
}

qreal CrawlWindow::threeStarsTravelSpeed() const {
    if (m_phase != Phase::ThreeStars)
        return 0.0;

    if (m_threeStarsStage == ThreeStarsStage::Travel) {
        const qreal t = static_cast<qreal>(m_threeStarsTick) / kThreeStarsTravelTicks;
        const qreal launch = easeInQuad(clamp01(t / 0.35));
        const qreal cruise = std::sin(clamp01(t) * kPi);
        return std::max(launch, cruise);
    }

    if (m_threeStarsStage == ThreeStarsStage::Reveal) {
        const qreal t = static_cast<qreal>(m_threeStarsTick) / kThreeStarsRevealTicks;
        return 0.36 * (1.0 - easeOutCubic(t));
    }

    if (m_threeStarsStage == ThreeStarsStage::Hold)
        return 0.0;

    return 0.0;
}
