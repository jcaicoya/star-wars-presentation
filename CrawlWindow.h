#pragma once

#include <QElapsedTimer>
#include <QFont>
#include <QImage>
#include <QTimer>
#include <QVector3D>
#include <QWidget>
#include <functional>
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
        ThreeStars,
        Planet
    };

    explicit CrawlWindow(QWidget *parent = nullptr);

    void setContent(const CrawlContent &content);
    void setGoalStars(const std::vector<StarDefinition> &stars);
    void setShowMode(ShowMode mode);
    void openShowWindow(bool fullscreen = false);

    std::function<void()> onClosed;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    enum class LineAlignment { Center, Left };
    enum class ThreeStarsStage {
        Entry,
        Acquire,
        Travel,
        Reveal,
        Hold,
        Transition
    };

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
    void tickSpaceflight();
    void tickThreeStars();

    // Per-phase paint (draws current phase on top of the starfield)
    void paintStarfield(QPainter &painter);
    void paintIntro(QPainter &painter);
    void paintLogo(QPainter &painter);
    void paintCrawl(QPainter &painter);
    void paintSpaceflight(QPainter &painter);
    void paintThreeStars(QPainter &painter);
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

    // ── Three-stars helpers ──────────────────────────────────────────────────
    QPointF targetPointForMessage(int index) const;
    QPointF starfieldFocusPoint() const;
    QPointF threeStarsCameraOffset() const;
    QPointF adjustedStarPoint(int index) const;
    QPointF acquisitionShiftDirection() const;
    QRectF  messageRect() const;
    qreal   threeStarsTravelSpeed() const;
    void    tickPlanet();
    void    paintPlanet(QPainter &painter);

    // ── Persistent data ──────────────────────────────────────────────────────
    CrawlContent      m_content;
    std::vector<Star> m_stars;
    std::vector<StarMessage> m_starMessages;
    std::vector<SpaceStar>   m_spaceStars;
    std::vector<StarDefinition> m_goalStars;
    QTimer            m_animationTimer;
    QElapsedTimer     m_elapsedTimer;
    bool              m_hasInitializedWindowGeometry = false;
    ShowMode          m_showMode = ShowMode::VideoGame;

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

    // Spaceflight phase
    QVector3D m_shipPosition;
    QVector3D m_shipVelocity;
    int       m_spaceflightTick = 0;
    qreal     m_spaceflightFade = 0.0;
    qreal     m_liveCrawlOverlayOpacity = 0.0;
    int       m_liveAutoTargetIndex = 0;
    bool      m_liveTargetReached = false;
    QVector3D m_liveLegStart;
    QVector3D m_liveLegEnd;
    int       m_liveLegTick = 0;
    int       m_liveLegDuration = 0;
    bool      m_liveLegActive = false;
    bool      m_moveLeft        = false;
    bool      m_moveRight       = false;
    bool      m_moveUp          = false;
    bool      m_moveDown        = false;
    bool      m_moveForward     = false;
    bool      m_moveBackward    = false;

    // Three-stars phase
    ThreeStarsStage m_threeStarsStage      = ThreeStarsStage::Entry;
    int             m_threeStarsTick       = 0;
    int             m_currentMessageIndex  = 0;
    int             m_previousMessageIndex = 0;
    qreal           m_threeStarsEntryFade  = 0.0;
    qreal           m_threeStarsTravel     = 0.0;
    qreal           m_threeStarsMessageOpacity = 0.0;
    qreal           m_threeStarsMessageScale   = 0.97;

    // Planet finale
    int   m_planetTick           = 0;
    qreal m_planetPanProgress    = 0.0;
    qreal m_planetApproach       = 0.0;
    qreal m_planetTextOpacity    = 0.0;
};
