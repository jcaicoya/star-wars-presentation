#pragma once

#include <QElapsedTimer>
#include <QFont>
#include <QImage>
#include <QTimer>
#include <QVector3D>
#include <QWidget>
#include <vector>

#include "CrawlContent.h"

class QPainter;

class CrawlWindow final : public QWidget {
    Q_OBJECT

public:
    enum class ShowMode {
        Live,
        VideoGame
    };

    enum class Phase {
        Intro,
        Logo,
        Crawl,
        Spaceflight,
        Hyperspace,
        Outro
    };

    enum class HyperspaceMode {
        Tunnel,
        Particles
    };

    explicit CrawlWindow(QWidget *parent = nullptr);

    void setContent(const CrawlContent &content);
    void setGoalStars(const std::vector<StarDefinition> &stars);
    void setShowMode(ShowMode mode);
    void setHyperspaceMode(HyperspaceMode mode);
    void openShowWindow(bool fullscreen, QSize windowSize = {});

signals:
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
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

    struct StarMessage {
        QString text;
        QPointF normalizedPosition;
        QColor  coreColor;
        QColor  glowColor;
    };

    struct SpaceStar {
        QVector3D position;
        qreal     radius = 1.0;
        qreal     twinklePhase = 0.0;
        QColor    color;
    };

    struct InputState {
        bool left     = false;
        bool right    = false;
        bool up       = false;
        bool down     = false;
        bool forward  = false;
        bool backward = false;
    };

    struct LiveFlightState {
        int       autoTargetIndex = 0;
        bool      targetReached   = false;
        QVector3D legStart;
        QVector3D legEnd;
        int       legTick     = 0;
        int       legDuration = 0;
        bool      legActive   = false;
    };

    // ── Viewport / scroll ────────────────────────────────────────────────────
    QRectF crawlViewport() const;
    qreal  scrollStep() const;

    // ── Starfield ────────────────────────────────────────────────────────────
    void regenerateStars();

    // ── Phase management ─────────────────────────────────────────────────────
    void transitionTo(Phase phase);
    void advanceToNextPhase();
    void retreatToPreviousPhase();

    // Per-phase tick (advances animation state; called every ~16 ms)
    void tickIntro();
    void tickLogo();
    void tickCrawl();
    void tickSpaceflight();
    void tickHyperspace();
    void tickOutro();

    // Per-phase paint (draws current phase on top of the starfield)
    void paintStarfield(QPainter &painter);
    void paintIntro(QPainter &painter);
    void paintLogo(QPainter &painter);
    void paintCrawl(QPainter &painter);
    void paintSpaceflight(QPainter &painter);
    void paintHyperspace(QPainter &painter);
    void paintOutro(QPainter &painter);
    void paintHUD(QPainter &painter);

    // ── Crawl-phase helpers ──────────────────────────────────────────────────
    void  rebuildLines();
    void  appendLine(const QString &text, const QFont &font,
                     LineAlignment alignment, qreal spacingAfter);
    void  renderCrawlImage();
    qreal totalContentHeight() const;

    // ── Spaceflight helpers ──────────────────────────────────────────────────
    void    initializeSpaceflightScene();
    void    recycleSpaceStars();
    void    paintSpaceScene(QPainter &painter, bool showGoalText, bool showHudCube, qreal crawlOverlayOpacity = 0.0);
    QPointF projectSpacePoint(const QVector3D &worldPoint, qreal *scale = nullptr) const;
    int     activeGoalStarIndex() const;

    // ── Persistent data ──────────────────────────────────────────────────────
    CrawlContent      m_content;
    std::vector<Star> m_stars;
    std::vector<StarMessage> m_starMessages;
    std::vector<SpaceStar>   m_spaceStars;
    std::vector<StarDefinition> m_goalStars;
    QTimer            m_animationTimer;
    QTimer            m_resizeDebounce;
    QElapsedTimer     m_elapsedTimer;
    bool              m_hasInitializedWindowGeometry = false;
    ShowMode          m_showMode = ShowMode::VideoGame;

    // ── Phase state ──────────────────────────────────────────────────────────
    Phase m_phase = Phase::Intro;

    // Intro phase
    int m_introTick = 0;

    // Logo phase
    int m_logoTick = 0;

    // Crawl phase
    std::vector<RenderLine> m_renderLines;
    QImage                  m_crawlImage;
    qreal                   m_sourceWindowHeight  = 0.0;
    qreal                   m_crawlOffset         = 0.0;
    qreal                   m_crawlTriggerOffset  = 0.0;
    qreal                   m_cameraTilt          = 0.0;
    qreal                   m_starDriftY          = 0.0;

    // Spaceflight phase
    QVector3D       m_shipPosition;
    QVector3D       m_shipVelocity;
    int             m_spaceflightTick = 0;
    qreal           m_spaceflightFade = 0.0;
    qreal           m_liveCrawlOverlayOpacity = 0.0;
    InputState      m_input;
    LiveFlightState m_liveFlight;

    // Hyperspace phase
    struct HyperParticle {
        qreal dirX;
        qreal dirY;
        qreal distance;
        qreal speed;
        qreal radius;
        qreal alpha;
        QColor color;
    };

    HyperspaceMode m_hyperspaceMode = HyperspaceMode::Tunnel;
    int   m_hyperspaceTick = 0;
    QPointF m_hyperspaceShakeOffset;
    std::vector<HyperParticle> m_hyperParticles;

    // Outro phase
    int   m_outroTick = 0;
};
