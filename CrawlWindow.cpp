#include "CrawlWindow.h"

#include <QCloseEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QPaintEvent>
#include <QPainter>
#include <QPolygonF>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QTransform>

#include <algorithm>
#include <cmath>

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
        case Phase::Intro: tickIntro(); break;
        case Phase::Logo:  tickLogo();  break;
        case Phase::Crawl: tickCrawl(); break;
        case Phase::Outro: tickOutro(); break;
        }
        update();
    });
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
    case Phase::Intro: paintIntro(painter); break;
    case Phase::Logo:  paintLogo(painter);  break;
    case Phase::Crawl: paintCrawl(painter); break;
    case Phase::Outro: paintOutro(painter); break;
    }

    paintHUD(painter);
}

void CrawlWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    regenerateStars();
    if (m_phase == Phase::Crawl)
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
        rebuildLines();
        break;
    case Phase::Outro:
        m_outroTick = 0;
        break;
    }
}

// ── Per-phase tick ────────────────────────────────────────────────────────────

void CrawlWindow::tickIntro() {
    // TODO: fade-in/out "A long time ago, in a galaxy far, far away..."
    transitionTo(Phase::Logo);
}

void CrawlWindow::tickLogo() {
    // TODO: Star Wars logo zoom-out from center
    transitionTo(Phase::Crawl);
}

void CrawlWindow::tickCrawl() {
    m_crawlOffset += scrollStep();
}

void CrawlWindow::tickOutro() {
    // TODO: camera pan / planet reveal
    m_animationTimer.stop();
}

// ── Per-phase paint ───────────────────────────────────────────────────────────

void CrawlWindow::paintStarfield(QPainter &painter) {
    painter.fillRect(rect(), QColor(0, 0, 0));
    painter.setPen(Qt::NoPen);
    for (const Star &star : m_stars) {
        painter.setBrush(star.color);
        painter.drawEllipse(star.position, star.radius, star.radius);
    }
}

void CrawlWindow::paintIntro(QPainter &painter) {
    // TODO: render intro text with fade in/out
    Q_UNUSED(painter);
}

void CrawlWindow::paintLogo(QPainter &painter) {
    // TODO: render Star Wars logo with zoom-out effect
    Q_UNUSED(painter);
}

void CrawlWindow::paintCrawl(QPainter &painter) {
    if (m_crawlImage.isNull())
        return;

    const QRectF viewport    = crawlViewport();
    const qreal topScreenY   = viewport.top()    + viewport.height() * 0.14;
    const qreal bottomScreenY= viewport.bottom() + viewport.height() * 0.02;
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

void CrawlWindow::paintOutro(QPainter &painter) {
    // TODO: render camera pan / planet
    Q_UNUSED(painter);
}

void CrawlWindow::paintHUD(QPainter &painter) {
    painter.setPen(QColor(180, 180, 180));
    painter.setFont(QFont(QStringLiteral("Consolas"), 11));
    painter.drawText(QRectF(20.0, height() - 36.0, width() - 40.0, 20.0),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Esc to editor   F11 fullscreen"));
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
