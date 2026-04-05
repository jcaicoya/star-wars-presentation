#include "CrawlWindow.h"

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

namespace {

constexpr qreal kPi = 3.14159265358979323846;

qreal clamp01(const qreal value) {
    return std::clamp(value, 0.0, 1.0);
}

qreal easeInOutCubic(const qreal t) {
    const qreal x = clamp01(t);
    return (x < 0.5) ? 4.0 * x * x * x : 1.0 - std::pow(-2.0 * x + 2.0, 3.0) / 2.0;
}

qreal easeOutCubic(const qreal t) {
    const qreal x = 1.0 - clamp01(t);
    return 1.0 - x * x * x;
}

qreal easeOutQuad(const qreal t) {
    const qreal x = clamp01(t);
    return 1.0 - (1.0 - x) * (1.0 - x);
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
        case Phase::ThreeStars: tickThreeStars(); break;
        }
        update();
    });

    m_starMessages = {
        {
            QStringLiteral("Move fast. Don't run."),
            QPointF(0.50, 0.48),
            QColor(230, 242, 255),
            QColor(130, 190, 255, 210)
        },
        {
            QStringLiteral("Create more than before."),
            QPointF(0.36, 0.34),
            QColor(255, 246, 220),
            QColor(255, 205, 120, 220)
        },
        {
            QStringLiteral("The purpose is people."),
            QPointF(0.67, 0.38),
            QColor(255, 238, 215),
            QColor(255, 176, 120, 215)
        }
    };
}

// ── Public interface ──────────────────────────────────────────────────────────

void CrawlWindow::setContent(const CrawlContent &content) {
    m_content = content;
}

void CrawlWindow::openShowWindow() {
    if (!m_hasInitializedWindowGeometry) {
        resize(1280, 720);
        m_hasInitializedWindowGeometry = true;
    }
    showNormal();
    raise();
    activateWindow();
    m_elapsedTimer.start();
    transitionTo(Phase::Intro);
    m_animationTimer.start();
    update();
}

// ── Qt overrides ──────────────────────────────────────────────────────────────

void CrawlWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    paintStarfield(painter);

    switch (m_phase) {
    case Phase::Intro:      paintIntro(painter);      break;
    case Phase::Logo:       paintLogo(painter);       break;
    case Phase::Crawl:      paintCrawl(painter);      break;
    case Phase::ThreeStars: paintThreeStars(painter); break;
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
    QWidget::keyPressEvent(event);
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
    case Phase::ThreeStars:
        m_threeStarsStage          = ThreeStarsStage::Entry;
        m_threeStarsTick           = 0;
        m_currentMessageIndex      = 0;
        m_threeStarsEntryFade      = 0.0;
        m_threeStarsTravel         = 0.0;
        m_threeStarsMessageOpacity = 0.0;
        m_threeStarsMessageScale   = 0.97;
        m_cameraTilt               = 1.0;
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
        transitionTo(Phase::ThreeStars);
        break;
    case Phase::ThreeStars:
        if (m_threeStarsStage == ThreeStarsStage::Hold) {
            m_threeStarsStage = ThreeStarsStage::Transition;
            m_threeStarsTick  = 0;
        }
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
    if (m_crawlOffset >= m_crawlTriggerOffset)
        transitionTo(Phase::ThreeStars);
}

static constexpr int kThreeStarsEntryTicks      = 54;
static constexpr int kThreeStarsAcquireTicks    = 34;
static constexpr int kThreeStarsTravelTicks     = 118;
static constexpr int kThreeStarsRevealTicks     = 34;
static constexpr int kThreeStarsTransitionTicks = 44;

void CrawlWindow::tickThreeStars() {
    ++m_threeStarsTick;

    switch (m_threeStarsStage) {
    case ThreeStarsStage::Entry:
        m_threeStarsEntryFade = easeOutQuad(static_cast<qreal>(m_threeStarsTick) / kThreeStarsEntryTicks);
        if (m_threeStarsTick >= kThreeStarsEntryTicks) {
            m_threeStarsStage = ThreeStarsStage::Acquire;
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
        if (m_threeStarsTick >= kThreeStarsRevealTicks) {
            m_threeStarsStage          = ThreeStarsStage::Hold;
            m_threeStarsTick           = 0;
            m_threeStarsMessageOpacity = 1.0;
            m_threeStarsMessageScale   = 1.0;
        }
        break;

    case ThreeStarsStage::Hold:
        break;

    case ThreeStarsStage::Transition: {
        const qreal t = static_cast<qreal>(m_threeStarsTick) / kThreeStarsTransitionTicks;
        const qreal eased = easeInOutCubic(t);
        m_threeStarsTravel         = 1.0 - eased;
        m_threeStarsMessageOpacity = 1.0 - easeOutQuad(t);
        m_threeStarsMessageScale   = 1.0 + 0.02 * eased;

        if (m_threeStarsTick >= kThreeStarsTransitionTicks) {
            ++m_currentMessageIndex;
            if (m_currentMessageIndex >= static_cast<int>(m_starMessages.size())) {
                m_animationTimer.stop();
                close();
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

// ── Per-phase paint ───────────────────────────────────────────────────────────

void CrawlWindow::paintStarfield(QPainter &painter) {
    painter.fillRect(rect(), QColor(0, 0, 0));
    painter.setPen(Qt::NoPen);
    const qreal h = std::max(1, height());
    const bool threeStarsActive = (m_phase == Phase::ThreeStars && m_currentMessageIndex < static_cast<int>(m_starMessages.size()));
    const QPointF targetPoint = threeStarsActive ? targetPointForMessage(m_currentMessageIndex)
                                                 : QPointF(width() * 0.5, height() * 0.5);
    const qreal travel = threeStarsActive ? m_threeStarsTravel : 0.0;

    for (const Star &star : m_stars) {
        // Apply vertical drift with wrapping so stars tile seamlessly
        qreal y = star.position.y() - m_starDriftY;
        y -= std::floor(y / h) * h;
        QPointF point(star.position.x(), y);
        qreal radius = star.radius;
        QColor color = star.color;

        if (threeStarsActive) {
            const QPointF delta = point - targetPoint;
            point += delta * (0.45 * travel);
            radius *= 1.0 + travel * 0.6;

            const qreal distance = std::hypot(delta.x(), delta.y());
            const qreal emphasis = clamp01(1.0 - distance / std::max<qreal>(220.0, width() * 0.24));
            color.setAlpha(std::clamp(static_cast<int>(120 + (100 * (1.0 - travel) + 35 * emphasis)), 0, 255));
        }

        painter.setBrush(color);
        painter.drawEllipse(point, radius, radius);
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

void CrawlWindow::paintThreeStars(QPainter &painter) {
    if (m_currentMessageIndex >= static_cast<int>(m_starMessages.size()))
        return;

    const StarMessage &message = m_starMessages[std::clamp(m_currentMessageIndex, 0, static_cast<int>(m_starMessages.size()) - 1)];
    const QPointF target = targetPointForMessage(m_currentMessageIndex);

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
    const qreal haloRadius = std::max(width(), height()) * (0.035 + 0.18 * travel);
    const qreal coreRadius = 3.0 + 5.0 * acquisition + 28.0 * travel;
    const qreal glowAlpha  = std::clamp(90 + static_cast<int>(110 * acquisition + 35 * travel), 0, 255);

    QRadialGradient halo(target, haloRadius, target);
    halo.setColorAt(0.00, QColor(message.glowColor.red(), message.glowColor.green(), message.glowColor.blue(), glowAlpha));
    halo.setColorAt(0.18, QColor(message.glowColor.red(), message.glowColor.green(), message.glowColor.blue(), glowAlpha / 2));
    halo.setColorAt(1.00, QColor(0, 0, 0, 0));
    painter.fillRect(rect(), halo);

    if (travel > 0.05) {
        const qreal vignetteAlpha = std::min(0.55, travel * 0.45);
        painter.fillRect(rect(), QColor(0, 0, 0, static_cast<int>(255 * vignetteAlpha)));
        painter.fillRect(rect(), halo);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(message.coreColor);
    painter.drawEllipse(target, coreRadius, coreRadius);

    if (m_currentMessageIndex == 2) {
        painter.setBrush(QColor(255, 210, 170, 170));
        const qreal orbitRadius = std::max(width(), height()) * (0.04 + 0.04 * travel);
        for (int i = 0; i < 4; ++i) {
            const qreal angle = 0.45 + i * 0.7;
            const QPointF neighbor(
                target.x() + std::cos(angle * kPi) * orbitRadius,
                target.y() + std::sin(angle * kPi) * orbitRadius * 0.55);
            painter.drawEllipse(neighbor, 1.8 + travel * 2.2, 1.8 + travel * 2.2);
        }
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
    const QString hint = (m_phase == Phase::ThreeStars && m_threeStarsStage == ThreeStarsStage::Hold)
        ? QStringLiteral("Esc to editor   F11 fullscreen   Space \u2192 next star")
        : QStringLiteral("Esc to editor   F11 fullscreen   Space \u2192 next scene");
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
        star.radius   = 0.6 + random->bounded(160) / 100.0;
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

QPointF CrawlWindow::targetPointForMessage(const int index) const {
    if (m_starMessages.empty())
        return QPointF(width() * 0.5, height() * 0.5);

    const StarMessage &message = m_starMessages[std::clamp(index, 0, static_cast<int>(m_starMessages.size()) - 1)];
    return QPointF(message.normalizedPosition.x() * width(), message.normalizedPosition.y() * height());
}

QRectF CrawlWindow::messageRect() const {
    const qreal rectWidth  = width() * 0.66;
    const qreal rectHeight = height() * 0.18;
    return QRectF((width() - rectWidth) * 0.5,
                  height() * 0.69,
                  rectWidth,
                  rectHeight);
}
