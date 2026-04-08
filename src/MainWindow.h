#pragma once

#include <QActionGroup>
#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QStackedWidget;
class QToolButton;
class CrawlWindow;
class LineNumberTextEdit;
class StarsEditorWidget;
struct CrawlContent;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    enum class StartupMode {
        Live,
        VideoGame
    };

    MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QString currentEditingStatus() const;
    void    activateMode(StartupMode mode);
    void    applyStartupDefaults();
    void    buildTextEditorPage();
    void    buildStarsEditorPage();
    void    buildLauncherPage();
    void    configureAction(QAction *action, const QString &text, const QKeySequence &shortcut);
    void         loadIntoEditor();
    CrawlContent collectContent() const;
    void         updateStatusLabels();
    void         updateBodyColumnGuide();
    void    configureCrawlWindow(CrawlWindow *window);
    void    ensureCrawlWindow();
    bool    saveEditorContents();
    bool    confirmDiscardOrSave();
    void    openEditor(int tabIndex);
    bool    leaveEditor();
    void    enterShowMode();
    void    refreshModeUi();

    QStackedWidget *m_pages              = nullptr;
    QWidget        *m_launcherPage       = nullptr;
    QWidget        *m_textEditorPage     = nullptr;
    QWidget        *m_starsEditorPage    = nullptr;
    QPlainTextEdit *m_introEdit          = nullptr;
    QPlainTextEdit *m_logoEdit           = nullptr;
    QLineEdit      *m_titleEdit          = nullptr;
    QLineEdit      *m_subtitleEdit       = nullptr;
    LineNumberTextEdit *m_bodyEdit        = nullptr;
    QPlainTextEdit *m_headerEdit         = nullptr;
    QPlainTextEdit *m_finalEdit          = nullptr;
    StarsEditorWidget *m_starsEditor     = nullptr;
    QLabel         *m_textStatusLabel    = nullptr;
    QLabel         *m_starsStatusLabel   = nullptr;
    QLabel         *m_launcherTitle   = nullptr;
    QLabel         *m_launcherSubtitle= nullptr;
    QToolButton    *m_liveCard        = nullptr;
    QToolButton    *m_videoGameCard   = nullptr;
    QToolButton    *m_editTextCard    = nullptr;
    QToolButton    *m_editStarsCard   = nullptr;
    QToolButton    *m_windowedCard    = nullptr;
    QToolButton    *m_fullscreenCard  = nullptr;
    QToolButton    *m_tunnelCard      = nullptr;
    QToolButton    *m_particlesCard   = nullptr;
    QAction        *m_liveAction      = nullptr;
    QAction        *m_videoGameAction = nullptr;
    QAction        *m_editTextAction  = nullptr;
    QAction        *m_editStarsAction = nullptr;
    QAction        *m_windowedAction  = nullptr;
    QAction        *m_fullscreenAction= nullptr;
    QAction        *m_tunnelAction    = nullptr;
    QAction        *m_particlesAction = nullptr;
    CrawlWindow    *m_crawlWindow     = nullptr;
    bool            m_hasUnsavedChanges = false;
    bool            m_textFormModified  = false;
    StartupMode     m_startupMode       = StartupMode::Live;
    bool            m_startFullscreen   = false;
    int             m_hyperspaceMode    = 0; // 0 = Tunnel, 1 = Particles
};
