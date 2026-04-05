#pragma once

#include <QElapsedTimer>
#include <QFont>
#include <QImage>
#include <QTimer>
#include <QWidget>
#include <functional>
#include <vector>

#include "CrawlContent.h"

class QPainter;

class CrawlWindow final : public QWidget {
    Q_OBJECT

public:
    // Sequential animation phases.
    // Each phase owns its own tick/paint methods and transitions to the next
    // when complete. Unimplemented phases skip forward immediately.
    enum class Phase {
        Intro,   // "A long time ago, in a galaxy far, far away..."
        Logo,    // Star Wars logo zooms out from center
        Crawl,   // Scrolling perspective text
        Outro    // Camera pans down to reveal a planet
    };

    explicit CrawlWindow(QWidget *parent = nullptr);

    void setContent(const CrawlContent &content);
    void openShowWindow();

    std::function<void()> onClosed;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    enum class LineAlignment { Center, Left };

    struct RenderLine {
        QString       text;
        QFont         font;
        LineAlignment alignment    = LineAlignment::Left;
        qreal         height       = 0.0;
        qreal         advance      = 0.0;
        qreal         spacingAfter = 0.0;
    };

    // ── Viewport / scroll ────────────────────────────────────────────────────
    QRectF crawlViewport() const;
    qreal  scrollStep() const;

    // ── Starfield ────────────────────────────────────────────────────────────
    void regenerateStars();

    // ── Phase management ─────────────────────────────────────────────────────
    void transitionTo(Phase phase);
    void advanceToNextPhase();

    // Per-phase tick (advances animation state; called every ~16 ms)
    void tickIntro();
    void tickLogo();
    void tickCrawl();
    void tickOutro();

    // Per-phase paint (draws current phase on top of the starfield)
    void paintStarfield(QPainter &painter);
    void paintIntro(QPainter &painter);
    void paintLogo(QPainter &painter);
    void paintCrawl(QPainter &painter);
    void paintOutro(QPainter &painter);
    void paintHUD(QPainter &painter);

    // ── Crawl-phase helpers ──────────────────────────────────────────────────
    void  rebuildLines();
    void  appendLine(const QString &text, const QFont &font,
                     LineAlignment alignment, qreal spacingAfter);
    void  renderCrawlImage();
    qreal totalContentHeight() const;

    // ── Persistent data ──────────────────────────────────────────────────────
    CrawlContent      m_content;
    std::vector<Star> m_stars;
    QTimer            m_animationTimer;
    QElapsedTimer     m_elapsedTimer;
    bool              m_hasInitializedWindowGeometry = false;

    // ── Phase state ──────────────────────────────────────────────────────────
    Phase m_phase = Phase::Intro;

    // Intro phase  (reserved for implementation)
    int m_introTick = 0;

    // Logo phase   (reserved for implementation)
    int m_logoTick = 0;

    // Crawl phase
    std::vector<RenderLine> m_renderLines;
    QImage                  m_crawlImage;
    qreal                   m_sourceWindowHeight  = 0.0;
    qreal                   m_crawlOffset         = 0.0;
    qreal                   m_crawlTriggerOffset  = 0.0; // offset at which outro is triggered
    qreal                   m_cameraTilt          = 0.0; // 0 = normal, 1 = fully tilted down
    qreal                   m_starDriftY          = 0.0; // cumulative upward star drift (camera pan)

    // Outro phase  (reserved for implementation)
    int m_outroTick = 0;
};
