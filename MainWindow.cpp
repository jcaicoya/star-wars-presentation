#include "MainWindow.h"
#include "CrawlWindow.h"
#include "TextIO.h"

#include <QAction>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTextDocument>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

// ── Construction ──────────────────────────────────────────────────────────────

MainWindow::MainWindow() {
    setWindowTitle(QStringLiteral("StarWarsText"));
    resize(1180, 940);

    auto *toolbar = addToolBar(QStringLiteral("Modes"));
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(20, 20));
    toolbar->setStyleSheet(
        "QToolBar {"
        "spacing: 8px;"
        "padding: 10px 12px;"
        "background: #0f1720;"
        "border: none;"
        "border-bottom: 1px solid #223245;"
        "}"
        "QToolButton {"
        "color: #d8e0ea;"
        "background: #162230;"
        "border: 1px solid #24384d;"
        "border-radius: 12px;"
        "padding: 9px 14px;"
        "font: 600 12pt 'Segoe UI';"
        "}"
        "QToolButton:checked {"
        "background: #d4ecff;"
        "color: #0d1b28;"
        "border-color: #d4ecff;"
        "}"
    );

    m_liveAction = toolbar->addAction(QStringLiteral("Live"));
    m_liveAction->setCheckable(true);
    m_editAction = toolbar->addAction(QStringLiteral("Edit"));
    m_editAction->setCheckable(true);
    m_videoGameAction = toolbar->addAction(QStringLiteral("Video game"));
    m_videoGameAction->setCheckable(true);

    auto *modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);
    modeGroup->addAction(m_liveAction);
    modeGroup->addAction(m_editAction);
    modeGroup->addAction(m_videoGameAction);

    toolbar->addSeparator();

    m_windowedAction = toolbar->addAction(QStringLiteral("Current size"));
    m_windowedAction->setCheckable(true);
    m_fullscreenAction = toolbar->addAction(QStringLiteral("Full screen"));
    m_fullscreenAction->setCheckable(true);

    auto *displayGroup = new QActionGroup(this);
    displayGroup->setExclusive(true);
    displayGroup->addAction(m_windowedAction);
    displayGroup->addAction(m_fullscreenAction);

    configureAction(m_liveAction, QStringLiteral("Live"), QKeySequence(QStringLiteral("Ctrl+L")));
    configureAction(m_editAction, QStringLiteral("Edit"), QKeySequence(QStringLiteral("Ctrl+E")));
    configureAction(m_videoGameAction, QStringLiteral("Video game"), QKeySequence(QStringLiteral("Ctrl+G")));
    configureAction(m_windowedAction, QStringLiteral("Current size"), QKeySequence(QStringLiteral("Ctrl+1")));
    configureAction(m_fullscreenAction, QStringLiteral("Full screen"), QKeySequence(QStringLiteral("Ctrl+2")));

    connect(m_liveAction, &QAction::triggered, this, [this]() { activateMode(StartupMode::Live); });
    connect(m_editAction, &QAction::triggered, this, [this]() { activateMode(StartupMode::Edit); });
    connect(m_videoGameAction, &QAction::triggered, this, [this]() { activateMode(StartupMode::VideoGame); });
    connect(m_windowedAction, &QAction::triggered, this, [this]() {
        m_startFullscreen = false;
        refreshModeUi();
    });
    connect(m_fullscreenAction, &QAction::triggered, this, [this]() {
        m_startFullscreen = true;
        refreshModeUi();
    });

    m_pages = new QStackedWidget(this);
    setCentralWidget(m_pages);

    buildLauncherPage();
    buildEditorPage();
    ensureCrawlWindow();

    connect(m_editor->document(), &QTextDocument::modificationChanged, this, [this](bool) {
        m_hasUnsavedChanges = m_editor->document()->isModified() || m_starsEditor->document()->isModified();
        setStatusForCurrentPath(currentEditingStatus());
    });
    connect(m_starsEditor->document(), &QTextDocument::modificationChanged, this, [this](bool) {
        m_hasUnsavedChanges = m_editor->document()->isModified() || m_starsEditor->document()->isModified();
        setStatusForCurrentPath(currentEditingStatus());
    });

    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, [this]() {
        saveEditorContents();
    });

    auto *fullscreenShortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(fullscreenShortcut, &QShortcut::activated, this, [this]() {
        if (m_crawlWindow != nullptr && m_crawlWindow->isVisible())
            m_crawlWindow->setWindowState(m_crawlWindow->windowState() ^ Qt::WindowFullScreen);
    });

    loadIntoEditor();
    applyStartupDefaults();
    setStatusForCurrentPath(QStringLiteral("Loaded"));
    refreshModeUi();
}

// ── Qt overrides ──────────────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!confirmDiscardOrSave()) {
        event->ignore();
        return;
    }
    if (m_crawlWindow != nullptr)
        m_crawlWindow->close();
    QMainWindow::closeEvent(event);
}

// ── Private helpers ───────────────────────────────────────────────────────────

QString MainWindow::currentEditingStatus() const {
    return m_hasUnsavedChanges ? QStringLiteral("Modified") : QStringLiteral("Editing");
}

void MainWindow::activateMode(const StartupMode mode) {
    m_startupMode = mode;
    refreshModeUi();
}

void MainWindow::applyStartupDefaults() {
    m_startupMode = StartupMode::Live;
    m_startFullscreen = false;
}

void MainWindow::buildEditorPage() {
    m_editorPage = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(m_editorPage);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    m_statusLabel = new QLabel(m_editorPage);
    m_statusLabel->setMinimumWidth(320);
    m_statusLabel->setStyleSheet("QLabel { color: #aab7c5; font: 11pt 'Segoe UI'; }");

    auto *launchButton = new QPushButton(QStringLiteral("Launch selected presentation"), m_editorPage);
    launchButton->setStyleSheet(
        "QPushButton {"
        "background: #f0b23b;"
        "color: #101824;"
        "border: none;"
        "border-radius: 12px;"
        "padding: 12px 18px;"
        "font: 700 12pt 'Segoe UI';"
        "}"
        "QPushButton:hover { background: #f6c157; }"
    );

    auto *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(10);
    toolbarLayout->addWidget(launchButton);
    toolbarLayout->addStretch(1);
    toolbarLayout->addWidget(m_statusLabel);

    const QString editorStyle =
        "QPlainTextEdit {"
        "background: #161616;"
        "color: #f1f1f1;"
        "border: 1px solid #303030;"
        "font: 16px 'Consolas';"
        "padding: 12px;"
        "selection-background-color: #404040;"
        "}"
    ;

    m_editorTabs = new QTabWidget(m_editorPage);
    m_editorTabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #2c3642; background: #10151b; }"
        "QTabBar::tab { background: #16202b; color: #bfcbd7; padding: 10px 16px; margin-right: 4px; border-top-left-radius: 8px; border-top-right-radius: 8px; }"
        "QTabBar::tab:selected { background: #24384d; color: #f4fbff; }"
    );

    m_editor = new QPlainTextEdit(m_editorPage);
    m_editor->setStyleSheet(editorStyle);
    m_starsEditor = new QPlainTextEdit(m_editorPage);
    m_starsEditor->setStyleSheet(editorStyle);

    m_editorTabs->addTab(m_editor, QStringLiteral("text.txt"));
    m_editorTabs->addTab(m_starsEditor, QStringLiteral("stars.json"));

    rootLayout->addLayout(toolbarLayout);
    rootLayout->addWidget(m_editorTabs, 1);

    connect(launchButton, &QPushButton::clicked, this, [this]() { enterShowMode(); });

    m_pages->addWidget(m_editorPage);
}

void MainWindow::buildLauncherPage() {
    m_launcherPage = new QWidget(this);
    m_launcherPage->setStyleSheet(
        "QWidget {"
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #08111a, stop:1 #132638);"
        "color: #f0f6fb;"
        "}"
        "QLabel#Title { font: 700 28pt 'Segoe UI'; color: #f4fbff; }"
        "QLabel#Subtitle { font: 13pt 'Segoe UI'; color: #a7bccd; }"
        "QToolButton {"
        "background: rgba(16, 28, 40, 0.86);"
        "border: 1px solid #27425b;"
        "border-radius: 22px;"
        "padding: 26px;"
        "font: 600 16pt 'Segoe UI';"
        "color: #e9f3fb;"
        "text-align: left;"
        "}"
        "QToolButton:checked {"
        "background: #d8efff;"
        "color: #0c1a27;"
        "border: 2px solid #d8efff;"
        "}"
        "QPushButton {"
        "background: #f0b23b;"
        "color: #101824;"
        "border: none;"
        "border-radius: 16px;"
        "padding: 16px 28px;"
        "font: 700 15pt 'Segoe UI';"
        "}"
        "QPushButton:hover { background: #f6c157; }"
    );

    auto *layout = new QVBoxLayout(m_launcherPage);
    layout->setContentsMargins(44, 40, 44, 40);
    layout->setSpacing(22);

    m_launcherTitle = new QLabel(QStringLiteral("Choose how to start"), m_launcherPage);
    m_launcherTitle->setObjectName(QStringLiteral("Title"));
    m_launcherSubtitle = new QLabel(
        QStringLiteral("Pick a mode and display style. Live mode is selected by default."),
        m_launcherPage);
    m_launcherSubtitle->setObjectName(QStringLiteral("Subtitle"));
    m_launcherSubtitle->setWordWrap(true);

    layout->addWidget(m_launcherTitle);
    layout->addWidget(m_launcherSubtitle);

    auto makeCard = [this](const QString &text, const int minHeight) {
        auto *button = new QToolButton(this);
        button->setCheckable(true);
        button->setText(text);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        button->setMinimumHeight(minHeight);
        return button;
    };

    auto *modeRow = new QHBoxLayout();
    modeRow->setSpacing(18);
    m_liveCard = makeCard(
        QStringLiteral("Live mode\n\nIntro, crawl, and the guided star sequence."),
        180);
    m_editCard = makeCard(
        QStringLiteral("Edit mode\n\nOpen the editor directly and refine the source text."),
        180);
    m_videoGameCard = makeCard(
        QStringLiteral("Video game mode\n\nSkip the crawl and pilot the ship through the star map."),
        180);

    auto *modeGroup = new QButtonGroup(this);
    modeGroup->setExclusive(true);
    modeGroup->addButton(m_liveCard);
    modeGroup->addButton(m_editCard);
    modeGroup->addButton(m_videoGameCard);

    connect(m_liveCard, &QToolButton::clicked, this, [this]() { activateMode(StartupMode::Live); });
    connect(m_editCard, &QToolButton::clicked, this, [this]() { activateMode(StartupMode::Edit); });
    connect(m_videoGameCard, &QToolButton::clicked, this, [this]() { activateMode(StartupMode::VideoGame); });

    modeRow->addWidget(m_liveCard);
    modeRow->addWidget(m_editCard);
    modeRow->addWidget(m_videoGameCard);
    layout->addLayout(modeRow, 1);

    auto *displayRow = new QHBoxLayout();
    displayRow->setSpacing(18);
    m_windowedCard = makeCard(
        QStringLiteral("Current size\n\nKeep the presentation window at its current size."),
        126);
    m_fullscreenCard = makeCard(
        QStringLiteral("Full screen\n\nOpen the presentation directly in full screen."),
        126);

    auto *displayGroup = new QButtonGroup(this);
    displayGroup->setExclusive(true);
    displayGroup->addButton(m_windowedCard);
    displayGroup->addButton(m_fullscreenCard);

    connect(m_windowedCard, &QToolButton::clicked, this, [this]() {
        m_startFullscreen = false;
        refreshModeUi();
    });
    connect(m_fullscreenCard, &QToolButton::clicked, this, [this]() {
        m_startFullscreen = true;
        refreshModeUi();
    });

    displayRow->addWidget(m_windowedCard);
    displayRow->addWidget(m_fullscreenCard);
    layout->addLayout(displayRow);

    auto *launchButton = new QPushButton(QStringLiteral("Open selected mode"), m_launcherPage);
    connect(launchButton, &QPushButton::clicked, this, [this]() {
        if (m_startupMode == StartupMode::Edit) {
            m_pages->setCurrentWidget(m_editorPage);
            refreshModeUi();
            return;
        }
        enterShowMode();
    });
    layout->addWidget(launchButton, 0, Qt::AlignLeft);

    m_pages->addWidget(m_launcherPage);
}

void MainWindow::configureAction(QAction *action, const QString &text, const QKeySequence &shortcut) {
    action->setText(text);
    action->setShortcut(shortcut);
    action->setShortcutContext(Qt::ApplicationShortcut);
}

void MainWindow::loadIntoEditor() {
    m_editor->setPlainText(loadRawText());
    m_starsEditor->setPlainText(loadRawStars());
    m_editor->document()->setModified(false);
    m_starsEditor->document()->setModified(false);
    m_hasUnsavedChanges = false;
}

void MainWindow::setStatusForCurrentPath(const QString &prefix, const QString &overridePath) {
    const QString path = overridePath.isEmpty() ? editableTextPath() : overridePath;
    m_statusLabel->setText(
        QStringLiteral("%1: %2").arg(prefix, path.isEmpty() ? resourceTextPath() : path));
}

void MainWindow::configureCrawlWindow(CrawlWindow *window) {
    connect(window, &CrawlWindow::destroyed, this, [this]() {
        m_crawlWindow = nullptr;
    });
    window->onClosed = [this]() {
        showNormal();
        raise();
        activateWindow();
        m_pages->setCurrentWidget(m_launcherPage);
        refreshModeUi();
        setStatusForCurrentPath(currentEditingStatus());
    };
}

void MainWindow::ensureCrawlWindow() {
    if (m_crawlWindow != nullptr)
        return;
    m_crawlWindow = new CrawlWindow();
    configureCrawlWindow(m_crawlWindow);
}

bool MainWindow::saveEditorContents() {
    QString textSavedPath;
    if (!saveRawText(m_editor->toPlainText(), &textSavedPath)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Save failed"),
            QStringLiteral("The text file could not be saved. The embedded Qt resource is read-only at runtime."));
        return false;
    }

    QString starsSavedPath;
    if (!saveRawStars(m_starsEditor->toPlainText(), &starsSavedPath)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Save failed"),
            QStringLiteral("The star configuration file could not be saved. The embedded Qt resource is read-only at runtime."));
        return false;
    }
    m_editor->document()->setModified(false);
    m_starsEditor->document()->setModified(false);
    m_hasUnsavedChanges = false;
    setStatusForCurrentPath(QStringLiteral("Saved"), textSavedPath);
    return true;
}

bool MainWindow::confirmDiscardOrSave() {
    if (!m_hasUnsavedChanges)
        return true;

    const auto choice = QMessageBox::question(
        this,
        QStringLiteral("Unsaved changes"),
        QStringLiteral("There are unsaved changes. Do you want to save them?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (choice == QMessageBox::Cancel) return false;
    if (choice == QMessageBox::Save)   return saveEditorContents();
    return true;
}

void MainWindow::enterShowMode() {
    ensureCrawlWindow();
    m_crawlWindow->setContent(parseContent(m_editor->toPlainText()));
    m_crawlWindow->setGoalStars(parseStars(m_starsEditor->toPlainText()));
    m_crawlWindow->setShowMode(
        m_startupMode == StartupMode::Live
            ? CrawlWindow::ShowMode::Live
            : CrawlWindow::ShowMode::VideoGame);
    hide();
    m_crawlWindow->openShowWindow(m_startFullscreen);
    setStatusForCurrentPath(QStringLiteral("Showing"));
}

void MainWindow::refreshModeUi() {
    if (m_liveAction != nullptr)       m_liveAction->setChecked(m_startupMode == StartupMode::Live);
    if (m_editAction != nullptr)       m_editAction->setChecked(m_startupMode == StartupMode::Edit);
    if (m_videoGameAction != nullptr)  m_videoGameAction->setChecked(m_startupMode == StartupMode::VideoGame);
    if (m_windowedAction != nullptr)   m_windowedAction->setChecked(!m_startFullscreen);
    if (m_fullscreenAction != nullptr) m_fullscreenAction->setChecked(m_startFullscreen);

    if (m_liveCard != nullptr)         m_liveCard->setChecked(m_startupMode == StartupMode::Live);
    if (m_editCard != nullptr)         m_editCard->setChecked(m_startupMode == StartupMode::Edit);
    if (m_videoGameCard != nullptr)    m_videoGameCard->setChecked(m_startupMode == StartupMode::VideoGame);
    if (m_windowedCard != nullptr)     m_windowedCard->setChecked(!m_startFullscreen);
    if (m_fullscreenCard != nullptr)   m_fullscreenCard->setChecked(m_startFullscreen);

    if (m_pages != nullptr) {
        if (m_startupMode == StartupMode::Edit)
            m_pages->setCurrentWidget(m_editorPage);
        else if (m_pages->currentWidget() != m_editorPage || !isHidden())
            m_pages->setCurrentWidget(m_launcherPage);
    }
}
