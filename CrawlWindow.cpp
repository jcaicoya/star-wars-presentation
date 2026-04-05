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
        case Phase::Planet:     tickPlanet();     break;
        }
        update();
    });

    m_starMessages = {
        {
            QStringLiteral("Move fast. Don't run."),
            QPointF(0.50, 0.50),
            QColor(230, 242, 255),
            QColor(130, 190, 255, 210)
        },
        {
            QStringLiteral("Create more than before."),
            QPointF(1.12, 0.50),
            QColor(255, 246, 220),
            QColor(255, 205, 120, 220)
        },
        {
            QStringLiteral("The purpose is people."),
            QPointF(0.50, -0.12),
            QColor(255, 238, 215),
            QColor(255, 176, 120, 215)
        }
    };
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
    case Phase::Planet:      paintPlanet(painter);     break;
    }

    paintHUD(painter);
}

void CrawlWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    regenerateStars();
    if (m_phase == Phase::Crawl || m_phase == Phase::ThreeStars)
        rebuildLines();
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
        case Qt::Key_Left:  m_moveLeft = true;     return;
        case Qt::Key_Right: m_moveRight = true;    return;
        case Qt::Key_Up:    m_moveUp = true;       return;
        case Qt::Key_Down:  m_moveDown = true;     return;
        case Qt::Key_W:     m_moveForward = true;  return;
        case Qt::Key_S:     m_moveBackward = true; return;
        default: break;
        }
    }
    QWidget::keyPressEvent(event);
}

void CrawlWindow::keyReleaseEvent(QKeyEvent *event) {
    if (m_phase == Phase::Spaceflight) {
        switch (event->key()) {
        case Qt::Key_Left:  m_moveLeft = false;     return;
        case Qt::Key_Right: m_moveRight = false;    return;
        case Qt::Key_Up:    m_moveUp = false;       return;
        case Qt::Key_Down:  m_moveDown = false;     return;
        case Qt::Key_W:     m_moveForward = false;  return;
        case Qt::Key_S:     m_moveBackward = false; return;
        default: break;
        }
    }
    QWidget::keyReleaseEvent(event);
}

void CrawlWindow::closeEvent(QCloseEvent *event) {
    m_animationTimer.stop();
    if (onClosed)
        onClosed();
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
        m_liveAutoTargetIndex = 0;
        m_liveTargetReached = false;
        m_liveLegStart = m_shipPosition;
        m_liveLegEnd = m_shipPosition;
        m_liveLegTick = 0;
        m_liveLegDuration = 0;
        m_liveLegActive = false;
        m_moveLeft = false;
        m_moveRight = false;
        m_moveUp = false;
        m_moveDown = false;
        m_moveForward = false;
        m_moveBackward = false;
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
    case Phase::Planet:
        m_planetTick        = 0;
        m_planetPanProgress = 0.0;
        m_planetApproach    = 0.0;
        m_planetTextOpacity = 0.0;
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
        if (m_showMode == ShowMode::Live && m_liveTargetReached) {
            if (m_liveAutoTargetIndex + 1 < static_cast<int>(m_goalStars.size())) {
                ++m_liveAutoTargetIndex;
                m_liveTargetReached = false;
                m_liveLegStart = m_shipPosition;
                m_liveLegEnd = m_goalStars[m_liveAutoTargetIndex].position + QVector3D(0.0f, 15.0f, -150.0f);
                m_liveLegTick = 0;
                const float distance = (m_liveLegEnd - m_liveLegStart).length();
                m_liveLegDuration = std::max(120, static_cast<int>(distance / 5.5f));
                m_liveLegActive = true;
            } else {
                transitionTo(Phase::Planet);
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
    case Phase::Planet:
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
    m_spaceflightFade = std::min(1.0, static_cast<qreal>(m_spaceflightTick) / 36.0);

    if (m_showMode == ShowMode::Live) {
        if (!m_liveLegActive && !m_liveTargetReached && m_liveAutoTargetIndex < static_cast<int>(m_goalStars.size())) {
            m_liveLegStart = m_shipPosition;
            m_liveLegEnd = m_goalStars[m_liveAutoTargetIndex].position + QVector3D(0.0f, 15.0f, -150.0f);
            m_liveLegTick = 0;
            const float distance = (m_liveLegEnd - m_liveLegStart).length();
            m_liveLegDuration = std::max(120, static_cast<int>(distance / 5.5f));
            m_liveLegActive = true;
        }

        if (m_liveLegActive) {
            ++m_liveLegTick;
            const qreal t = clamp01(static_cast<qreal>(m_liveLegTick) / std::max(1, m_liveLegDuration));
            const qreal eased = easeInOutSine(t);
            m_shipPosition = m_liveLegStart + (m_liveLegEnd - m_liveLegStart) * eased;
            m_shipVelocity = (m_liveLegEnd - m_liveLegStart) * static_cast<float>(1.0 / std::max(1, m_liveLegDuration));
            m_liveCrawlOverlayOpacity = std::max(0.0, m_liveCrawlOverlayOpacity - (1.0 / 48.0));

            if (m_liveLegTick >= m_liveLegDuration) {
                m_shipPosition = m_liveLegEnd;
                m_shipVelocity = QVector3D();
                m_liveLegActive = false;
                m_liveTargetReached = true;
            }
        }

        recycleSpaceStars();
        return;
    }

    QVector3D desired;
    if (m_moveLeft)     desired.setX(desired.x() - 1.0f);
    if (m_moveRight)    desired.setX(desired.x() + 1.0f);
    if (m_moveUp)       desired.setY(desired.y() - 1.0f);
    if (m_moveDown)     desired.setY(desired.y() + 1.0f);
    if (m_moveForward)  desired.setZ(desired.z() + 1.0f);
    if (m_moveBackward) desired.setZ(desired.z() - 1.0f);

    if (!desired.isNull())
        desired.normalize();

    if (!desired.isNull())
        desired.normalize();
    QVector3D acceleration(desired.x() * 1.8f, desired.y() * 1.5f, desired.z() * 2.8f);
    m_shipVelocity *= 0.88f;
    m_shipVelocity += acceleration;

    const float maxLateral = 7.5f;
    const float maxForward = 11.0f;
    m_shipVelocity.setX(std::clamp(m_shipVelocity.x(), -maxLateral, maxLateral));
    m_shipVelocity.setY(std::clamp(m_shipVelocity.y(), -maxLateral, maxLateral));
    m_shipVelocity.setZ(std::clamp(m_shipVelocity.z(), -maxForward, maxForward));
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
                transitionTo(Phase::Planet);
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

static constexpr int kPlanetPanTicks      = 150;
static constexpr int kPlanetTravelTicks   = 130;
static constexpr int kPlanetRevealTicks   = 50;

void CrawlWindow::tickPlanet() {
    ++m_planetTick;

    if (m_planetTick <= kPlanetPanTicks) {
        m_planetPanProgress = easeInOutSine(static_cast<qreal>(m_planetTick) / kPlanetPanTicks);
        return;
    }

    if (m_planetTick <= kPlanetPanTicks + kPlanetTravelTicks) {
        const int localTick = m_planetTick - kPlanetPanTicks;
        m_planetPanProgress = 1.0;
        m_planetApproach    = easeInOutCubic(static_cast<qreal>(localTick) / kPlanetTravelTicks);
        return;
    }

    const int revealTick = m_planetTick - kPlanetPanTicks - kPlanetTravelTicks;
    m_planetPanProgress = 1.0;
    m_planetApproach    = 1.0;
    m_planetTextOpacity = easeOutCubic(static_cast<qreal>(revealTick) / kPlanetRevealTicks);
}

// ── Per-phase paint ───────────────────────────────────────────────────────────

void CrawlWindow::paintStarfield(QPainter &painter) {
    painter.fillRect(rect(), QColor(0, 0, 0));
    painter.setPen(Qt::NoPen);
    const qreal w = std::max(1, width());
    const qreal h = std::max(1, height());
    const bool threeStarsActive = (m_phase == Phase::ThreeStars && m_currentMessageIndex < static_cast<int>(m_starMessages.size()));
    const bool planetActive = (m_phase == Phase::Planet);
    const QPointF targetPoint = threeStarsActive ? starfieldFocusPoint()
                                                 : QPointF(width() * 0.5, height() * 0.5);
    const qreal travel = threeStarsActive ? m_threeStarsTravel : 0.0;
    const qreal speed  = threeStarsActive ? threeStarsTravelSpeed() : (planetActive ? 0.65 * m_planetApproach : 0.0);
    const qreal phaseTime = m_elapsedTimer.isValid() ? static_cast<qreal>(m_elapsedTimer.elapsed()) / 1000.0 : 0.0;
    QPointF cameraOffset;
    if (threeStarsActive) {
        cameraOffset = threeStarsCameraOffset();
    } else if (planetActive) {
        cameraOffset = QPointF(width() * 0.24 * m_planetPanProgress, 0.0);
    }

    for (const Star &star : m_stars) {
        qreal x = star.position.x() + cameraOffset.x();
        qreal y = star.position.y() - m_starDriftY + cameraOffset.y();
        x -= std::floor(x / w) * w;
        y -= std::floor(y / h) * h;
        QPointF point(x, y);
        qreal radius = star.radius;
        QColor color = star.color;

        if (threeStarsActive || planetActive) {
            const QPointF delta = point - targetPoint;
            const qreal depthWeight = 0.35 + (1.0 - star.depth) * 1.7;
            const qreal travelPush = travel * (0.16 + 0.92 * easeOutCubic(travel)) * depthWeight;
            if (threeStarsActive)
                point += delta * travelPush;
            else
                point += delta * (m_planetApproach * (0.18 + 0.72 * easeOutCubic(m_planetApproach)) * depthWeight);
            radius *= 1.0 + (threeStarsActive ? travel : m_planetApproach) * 0.45 + speed * 0.22 * depthWeight;

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
        const qreal emphasis = clamp01(1.0 - distance / 700.0);
        const qreal radius = std::max<qreal>(0.9, goal.radius * 0.16 + scale * 16.0);
        const qreal glowRadius = radius * (2.4 + emphasis * 1.8);

        QRadialGradient glow(point, glowRadius, point);
        glow.setColorAt(0.0, QColor(goal.glowColor.red(), goal.glowColor.green(), goal.glowColor.blue(),
                                    std::clamp(static_cast<int>(90 + emphasis * 150), 0, 255)));
        glow.setColorAt(0.28, QColor(goal.glowColor.red(), goal.glowColor.green(), goal.glowColor.blue(),
                                     std::clamp(static_cast<int>(50 + emphasis * 90), 0, 255)));
        glow.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(rect(), glow);

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
        painter.fillRect(rect(), halo);

        if (starTravel > 0.05) {
            const qreal vignetteAlpha = std::min(0.55, starTravel * 0.45) * alphaScale;
            painter.fillRect(rect(), QColor(0, 0, 0, static_cast<int>(255 * vignetteAlpha)));
            painter.fillRect(rect(), halo);
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

    if (m_threeStarsMessageOpacity > 0.0) {
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

void CrawlWindow::paintPlanet(QPainter &painter) {
    const QPointF center(width() * 0.5, height() * 0.58);
    const QPointF worldPlanet(width() * 0.26, height() * 0.64);
    const QPointF cameraOffset(width() * 0.24 * m_planetPanProgress, 0.0);
    const QPointF planetCenter = worldPlanet + cameraOffset;

    const qreal radius = std::max(width(), height()) * (0.12 + 0.32 * m_planetApproach);
    const qreal glowRadius = radius * 1.55;

    QRadialGradient atmosphere(planetCenter, glowRadius, planetCenter - QPointF(radius * 0.22, radius * 0.28));
    atmosphere.setColorAt(0.00, QColor(160, 210, 255, static_cast<int>(210 * std::max(0.25, m_planetApproach))));
    atmosphere.setColorAt(0.55, QColor(70, 120, 210, static_cast<int>(150 * std::max(0.25, m_planetApproach))));
    atmosphere.setColorAt(1.00, QColor(0, 0, 0, 0));
    painter.fillRect(rect(), atmosphere);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(22, 46, 98));
    painter.drawEllipse(planetCenter, radius, radius);

    painter.setBrush(QColor(70, 122, 205, 220));
    painter.drawEllipse(planetCenter + QPointF(-radius * 0.08, radius * 0.02), radius * 0.9, radius * 0.9);

    painter.setBrush(QColor(210, 235, 255, 85));
    painter.drawEllipse(planetCenter + QPointF(-radius * 0.24, -radius * 0.22), radius * 0.48, radius * 0.32);

    const QRectF textRect(width() * 0.16, height() * 0.18, width() * 0.68, height() * 0.34);
    QFont titleFont(QStringLiteral("Arial"), std::max(22, static_cast<int>(height() * 0.045)), QFont::DemiBold);
    QFont bodyFont(QStringLiteral("Arial"), std::max(18, static_cast<int>(height() * 0.034)));

    painter.save();
    painter.setOpacity(m_planetTextOpacity);
    painter.setFont(titleFont);
    painter.setPen(QColor(245, 245, 240));
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("Three guiding stars"));

    painter.setFont(bodyFont);
    const QString summary =
        QStringLiteral("Move fast. Don't run.\n\n")
        + QStringLiteral("Create more than before.\n\n")
        + QStringLiteral("The purpose is people.");
    painter.drawText(textRect.adjusted(0.0, height() * 0.08, 0.0, 0.0),
                     Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                     summary);
    painter.restore();
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
    if (m_phase == Phase::Spaceflight && m_showMode == ShowMode::Live && m_liveTargetReached) {
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
    const int safeWidth  = std::max(1, width());
    const int safeHeight = std::max(1, height());
    const int starCount  = std::max(120, (safeWidth * safeHeight) / 9000);
    auto *random = QRandomGenerator::global();

    for (int i = 0; i < starCount; ++i) {
        Star star;
        star.position = QPointF(random->bounded(safeWidth), random->bounded(safeHeight));
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
    constexpr int kSpaceStarCount = 720;

    for (int i = 0; i < kSpaceStarCount; ++i) {
        SpaceStar star;
        star.position = QVector3D(
            static_cast<float>(random->bounded(-2600, 2601)),
            static_cast<float>(random->bounded(-1800, 1801)),
            static_cast<float>(random->bounded(-200, 5200)));
        star.radius = 0.12 + random->bounded(36) / 100.0;
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
    return (relative.length() < 240.0f) ? bestIndex : -1;
}

QPointF CrawlWindow::targetPointForMessage(const int index) const {
    if (m_starMessages.empty())
        return QPointF(width() * 0.5, height() * 0.5);

    const StarMessage &message = m_starMessages[std::clamp(index, 0, static_cast<int>(m_starMessages.size()) - 1)];
    return QPointF(message.normalizedPosition.x() * width(), message.normalizedPosition.y() * height());
}

QPointF CrawlWindow::starfieldFocusPoint() const {
    if (m_phase == Phase::Planet)
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
    if (m_phase == Phase::Planet)
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
