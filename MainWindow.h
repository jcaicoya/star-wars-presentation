#pragma once

#include <QActionGroup>
#include <QMainWindow>

class QLabel;
class QPlainTextEdit;
class QStackedWidget;
class QToolButton;
class CrawlWindow;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    enum class StartupMode {
        Live,
        Edit,
        VideoGame
    };

    MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QString currentEditingStatus() const;
    void    activateMode(StartupMode mode);
    void    applyStartupDefaults();
    void    buildEditorPage();
    void    buildLauncherPage();
    void    configureAction(QAction *action, const QString &text, const QKeySequence &shortcut);
    void    loadIntoEditor();
    void    setStatusForCurrentPath(const QString &prefix, const QString &overridePath = {});
    void    configureCrawlWindow(CrawlWindow *window);
    void    ensureCrawlWindow();
    bool    saveEditorContents();
    bool    confirmDiscardOrSave();
    void    enterShowMode();
    void    refreshModeUi();

    QStackedWidget *m_pages           = nullptr;
    QWidget        *m_launcherPage    = nullptr;
    QWidget        *m_editorPage      = nullptr;
    QPlainTextEdit *m_editor          = nullptr;
    QLabel         *m_statusLabel     = nullptr;
    QLabel         *m_launcherTitle   = nullptr;
    QLabel         *m_launcherSubtitle= nullptr;
    QToolButton    *m_liveCard        = nullptr;
    QToolButton    *m_editCard        = nullptr;
    QToolButton    *m_videoGameCard   = nullptr;
    QToolButton    *m_windowedCard    = nullptr;
    QToolButton    *m_fullscreenCard  = nullptr;
    QAction        *m_liveAction      = nullptr;
    QAction        *m_editAction      = nullptr;
    QAction        *m_videoGameAction = nullptr;
    QAction        *m_windowedAction  = nullptr;
    QAction        *m_fullscreenAction= nullptr;
    CrawlWindow    *m_crawlWindow     = nullptr;
    bool            m_hasUnsavedChanges = false;
    StartupMode     m_startupMode       = StartupMode::Live;
    bool            m_startFullscreen   = false;
};
