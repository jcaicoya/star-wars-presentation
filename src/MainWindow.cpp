#include "MainWindow.h"
#include "CrawlWindow.h"
#include "StarsEditorWidget.h"
#include "TextIO.h"

#include <QAction>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>

#include "LineNumberTextEdit.h"
#include <QPushButton>
#include <QShortcut>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

// ── Construction ──────────────────────────────────────────────────────────────

MainWindow::MainWindow() {
    setWindowTitle(QStringLiteral("StarWarsText"));
    setMinimumSize(800, 600);
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
    m_videoGameAction = toolbar->addAction(QStringLiteral("Video game"));
    m_videoGameAction->setCheckable(true);

    auto *modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);
    modeGroup->addAction(m_liveAction);
    modeGroup->addAction(m_videoGameAction);

    toolbar->addSeparator();

    m_fullscreenAction = toolbar->addAction(QStringLiteral("Full screen"));
    m_fullscreenAction->setCheckable(true);
    m_windowedAction = toolbar->addAction(QStringLiteral("Current size"));
    m_windowedAction->setCheckable(true);

    auto *displayGroup = new QActionGroup(this);
    displayGroup->setExclusive(true);
    displayGroup->addAction(m_fullscreenAction);
    displayGroup->addAction(m_windowedAction);

    configureAction(m_liveAction, QStringLiteral("Live"), QKeySequence(QStringLiteral("Ctrl+L")));
    configureAction(m_videoGameAction, QStringLiteral("Video game"), QKeySequence(QStringLiteral("Ctrl+G")));
    configureAction(m_fullscreenAction, QStringLiteral("Full screen"), QKeySequence(QStringLiteral("Ctrl+1")));
    configureAction(m_windowedAction, QStringLiteral("Current size"), QKeySequence(QStringLiteral("Ctrl+2")));

    toolbar->addSeparator();

    m_editTextAction = toolbar->addAction(QStringLiteral("Edit text"));
    m_editStarsAction = toolbar->addAction(QStringLiteral("Edit stars"));

    configureAction(m_editTextAction, QStringLiteral("Edit text"), QKeySequence(QStringLiteral("Ctrl+E")));
    configureAction(m_editStarsAction, QStringLiteral("Edit stars"), {});

    toolbar->addSeparator();

    m_tunnelAction = toolbar->addAction(QStringLiteral("Tunnel"));
    m_tunnelAction->setCheckable(true);
    m_particlesAction = toolbar->addAction(QStringLiteral("Particles"));
    m_particlesAction->setCheckable(true);

    auto *hyperGroup = new QActionGroup(this);
    hyperGroup->setExclusive(true);
    hyperGroup->addAction(m_tunnelAction);
    hyperGroup->addAction(m_particlesAction);

    configureAction(m_tunnelAction, QStringLiteral("Tunnel"), QKeySequence(QStringLiteral("Ctrl+3")));
    configureAction(m_particlesAction, QStringLiteral("Particles"), QKeySequence(QStringLiteral("Ctrl+4")));

    connect(m_tunnelAction, &QAction::triggered, this, [this]() {
        if (!leaveEditor()) return;
        m_hyperspaceMode = 0;
        refreshModeUi();
    });
    connect(m_particlesAction, &QAction::triggered, this, [this]() {
        if (!leaveEditor()) return;
        m_hyperspaceMode = 1;
        refreshModeUi();
    });

    connect(m_liveAction, &QAction::triggered, this, [this]() {
        if (!leaveEditor()) return;
        activateMode(StartupMode::Live);
    });
    connect(m_videoGameAction, &QAction::triggered, this, [this]() {
        if (!leaveEditor()) return;
        activateMode(StartupMode::VideoGame);
    });
    connect(m_editTextAction, &QAction::triggered, this, [this]() { openEditor(0); });
    connect(m_editStarsAction, &QAction::triggered, this, [this]() { openEditor(1); });
    connect(m_windowedAction, &QAction::triggered, this, [this]() {
        if (!leaveEditor()) return;
        m_startFullscreen = false;
        refreshModeUi();
    });
    connect(m_fullscreenAction, &QAction::triggered, this, [this]() {
        if (!leaveEditor()) return;
        m_startFullscreen = true;
        refreshModeUi();
    });

    m_pages = new QStackedWidget(this);
    setCentralWidget(m_pages);

    buildLauncherPage();
    buildTextEditorPage();
    buildStarsEditorPage();
    ensureCrawlWindow();

    auto trackTextChange = [this]() {
        m_textFormModified = true;
        m_hasUnsavedChanges = true;
        updateStatusLabels();
    };
    connect(m_introEdit, &QPlainTextEdit::textChanged, this, trackTextChange);
    connect(m_logoEdit, &QPlainTextEdit::textChanged, this, trackTextChange);
    connect(m_titleEdit, &QLineEdit::textChanged, this, trackTextChange);
    connect(m_subtitleEdit, &QLineEdit::textChanged, this, trackTextChange);
    connect(m_bodyEdit, &QPlainTextEdit::textChanged, this, trackTextChange);
    connect(m_bodyEdit, &QPlainTextEdit::textChanged, this, &MainWindow::updateBodyColumnGuide);
    connect(m_headerEdit, &QPlainTextEdit::textChanged, this, trackTextChange);
    connect(m_finalEdit, &QPlainTextEdit::textChanged, this, trackTextChange);
    connect(m_starsEditor, &StarsEditorWidget::modificationChanged, this, [this](bool) {
        m_hasUnsavedChanges = m_textFormModified || m_starsEditor->isModified();
        updateStatusLabels();
    });

    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, [this]() {
        saveEditorContents();
    });

    loadIntoEditor();
    applyStartupDefaults();
    updateStatusLabels();
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

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_F5 && m_pages->currentWidget() == m_launcherPage) {
        enterShowMode();
        return;
    }
    QMainWindow::keyPressEvent(event);
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
    m_startFullscreen = true;
}

void MainWindow::buildTextEditorPage() {
    m_textEditorPage = new QWidget(this);
    auto *root = new QVBoxLayout(m_textEditorPage);
    root->setContentsMargins(16, 12, 16, 12);
    root->setSpacing(8);

    m_textStatusLabel = new QLabel(m_textEditorPage);
    m_textStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #aab7c5; font: 11pt 'Segoe UI'; }"));
    root->addWidget(m_textStatusLabel);

    const QString multiStyle =
        "QPlainTextEdit {"
        "background: #161616;"
        "color: #f1f1f1;"
        "border: 1px solid #303030;"
        "font: 15px 'Consolas';"
        "padding: 8px;"
        "selection-background-color: #404040;"
        "}";

    const QString lineStyle =
        "QLineEdit {"
        "background: #161616;"
        "color: #f1f1f1;"
        "border: 1px solid #303030;"
        "font: 15px 'Consolas';"
        "padding: 8px;"
        "selection-background-color: #404040;"
        "}";

    const QString labelStyle = QStringLiteral(
        "QLabel { color: #7a8fa3; font: 600 10pt 'Segoe UI'; margin-top: 4px; }");

    auto makeLabel = [&](const QString &text, QLayout *target) {
        auto *label = new QLabel(text, m_textEditorPage);
        label->setStyleSheet(labelStyle);
        target->addWidget(label);
    };

    auto makeMultiEdit = [&](QPlainTextEdit *&field) {
        auto *edit = new LineNumberTextEdit(m_textEditorPage);
        edit->setStyleSheet(multiStyle);
        field = edit;
    };

    auto makeLineEdit = [&](QLineEdit *&field) {
        field = new QLineEdit(m_textEditorPage);
        field->setStyleSheet(lineStyle);
    };

    auto *columns = new QHBoxLayout();
    columns->setSpacing(16);

    // ── Left column: Intro, Logo, Title, Subtitle ────────────────────────
    auto *leftCol = new QVBoxLayout();
    leftCol->setSpacing(4);

    makeLabel(QStringLiteral("Intro"), leftCol);
    makeMultiEdit(m_introEdit);
    leftCol->addWidget(m_introEdit, 2);

    makeLabel(QStringLiteral("Logo"), leftCol);
    makeMultiEdit(m_logoEdit);
    leftCol->addWidget(m_logoEdit, 1);

    makeLabel(QStringLiteral("Title"), leftCol);
    makeLineEdit(m_titleEdit);
    leftCol->addWidget(m_titleEdit);

    makeLabel(QStringLiteral("Subtitle"), leftCol);
    makeLineEdit(m_subtitleEdit);
    leftCol->addWidget(m_subtitleEdit);

    columns->addLayout(leftCol, 1);

    // ── Center column: Body ──────────────────────────────────────────────
    auto *centerCol = new QVBoxLayout();
    centerCol->setSpacing(4);

    makeLabel(QStringLiteral("Body"), centerCol);
    m_bodyEdit = new LineNumberTextEdit(m_textEditorPage);
    m_bodyEdit->setStyleSheet(multiStyle);
    centerCol->addWidget(m_bodyEdit, 1);

    columns->addLayout(centerCol, 2);

    // ── Right column: Header, Final ──────────────────────────────────────
    auto *rightCol = new QVBoxLayout();
    rightCol->setSpacing(4);

    makeLabel(QStringLiteral("Header"), rightCol);
    makeMultiEdit(m_headerEdit);
    rightCol->addWidget(m_headerEdit, 1);

    makeLabel(QStringLiteral("Final"), rightCol);
    makeMultiEdit(m_finalEdit);
    rightCol->addWidget(m_finalEdit, 1);

    columns->addLayout(rightCol, 1);

    root->addLayout(columns, 1);
    m_pages->addWidget(m_textEditorPage);
}

void MainWindow::buildStarsEditorPage() {
    const QString statusStyle = QStringLiteral("QLabel { color: #aab7c5; font: 11pt 'Segoe UI'; }");

    m_starsEditorPage = new QWidget(this);
    auto *layout = new QVBoxLayout(m_starsEditorPage);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    m_starsStatusLabel = new QLabel(m_starsEditorPage);
    m_starsStatusLabel->setMinimumWidth(320);
    m_starsStatusLabel->setStyleSheet(statusStyle);
    layout->addWidget(m_starsStatusLabel);

    m_starsEditor = new StarsEditorWidget(m_starsEditorPage);
    layout->addWidget(m_starsEditor, 1);

    m_pages->addWidget(m_starsEditorPage);
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
    m_videoGameCard = makeCard(
        QStringLiteral("Video game mode\n\nSkip the crawl and pilot the ship through the star map."),
        180);

    auto *modeGroup = new QButtonGroup(this);
    modeGroup->setExclusive(true);
    modeGroup->addButton(m_liveCard);
    modeGroup->addButton(m_videoGameCard);

    connect(m_liveCard, &QToolButton::clicked, this, [this]() { activateMode(StartupMode::Live); });
    connect(m_videoGameCard, &QToolButton::clicked, this, [this]() { activateMode(StartupMode::VideoGame); });

    modeRow->addWidget(m_liveCard);
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

    displayRow->addWidget(m_fullscreenCard);
    displayRow->addWidget(m_windowedCard);
    layout->addLayout(displayRow);

    auto *editorRow = new QHBoxLayout();
    editorRow->setSpacing(18);
    m_editTextCard = makeCard(
        QStringLiteral("Edit text\n\nOpen the text editor to refine the crawl content."),
        126);
    m_editTextCard->setCheckable(false);
    m_editStarsCard = makeCard(
        QStringLiteral("Edit stars\n\nOpen the star editor to adjust the star map."),
        126);
    m_editStarsCard->setCheckable(false);

    connect(m_editTextCard, &QToolButton::clicked, this, [this]() { openEditor(0); });
    connect(m_editStarsCard, &QToolButton::clicked, this, [this]() { openEditor(1); });

    editorRow->addWidget(m_editTextCard);
    editorRow->addWidget(m_editStarsCard);
    layout->addLayout(editorRow);

    auto *hyperRow = new QHBoxLayout();
    hyperRow->setSpacing(18);
    m_tunnelCard = makeCard(
        QStringLiteral("Tunnel\n\nRadial light tunnel with blue-white gradient and screen flash."),
        126);
    m_particlesCard = makeCard(
        QStringLiteral("Particles\n\nParticle streams flying outward with trails and glow."),
        126);

    auto *hyperCardGroup = new QButtonGroup(this);
    hyperCardGroup->setExclusive(true);
    hyperCardGroup->addButton(m_tunnelCard);
    hyperCardGroup->addButton(m_particlesCard);

    connect(m_tunnelCard, &QToolButton::clicked, this, [this]() {
        m_hyperspaceMode = 0;
        refreshModeUi();
    });
    connect(m_particlesCard, &QToolButton::clicked, this, [this]() {
        m_hyperspaceMode = 1;
        refreshModeUi();
    });

    hyperRow->addWidget(m_tunnelCard);
    hyperRow->addWidget(m_particlesCard);
    layout->addLayout(hyperRow);

    auto *launchButton = new QPushButton(QStringLiteral("Open selected mode"), m_launcherPage);
    connect(launchButton, &QPushButton::clicked, this, [this]() { enterShowMode(); });
    layout->addWidget(launchButton, 0, Qt::AlignLeft);

    m_pages->addWidget(m_launcherPage);
}

void MainWindow::configureAction(QAction *action, const QString &text, const QKeySequence &shortcut) {
    action->setText(text);
    action->setShortcut(shortcut);
    action->setShortcutContext(Qt::ApplicationShortcut);
}

void MainWindow::loadIntoEditor() {
    const CrawlContent content = parseContent(loadRawText());
    m_introEdit->setPlainText(content.intro);
    m_logoEdit->setPlainText(content.logo);
    m_titleEdit->setText(content.title);
    m_subtitleEdit->setText(content.subtitle);
    m_bodyEdit->setPlainText(content.bodyLines.join(QLatin1Char('\n')));
    m_headerEdit->setPlainText(content.planetHeader);
    m_finalEdit->setPlainText(content.finalSentence);

    m_starsEditor->setStars(parseStars(loadRawStars()));

    m_textFormModified = false;
    m_hasUnsavedChanges = false;
}

CrawlContent MainWindow::collectContent() const {
    CrawlContent content;
    content.intro         = m_introEdit->toPlainText().trimmed();
    content.logo          = m_logoEdit->toPlainText().trimmed();
    content.title         = m_titleEdit->text().trimmed();
    content.subtitle      = m_subtitleEdit->text().trimmed();
    content.bodyLines     = m_bodyEdit->toPlainText().split(QLatin1Char('\n'));
    content.planetHeader  = m_headerEdit->toPlainText().trimmed();
    content.finalSentence = m_finalEdit->toPlainText().trimmed();
    return content;
}

void MainWindow::updateStatusLabels() {
    const QString prefix = currentEditingStatus();
    const QString textPath = editableTextPath();
    if (m_textStatusLabel != nullptr)
        m_textStatusLabel->setText(QStringLiteral("%1: %2").arg(prefix,
            textPath.isEmpty() ? resourceTextPath() : textPath));
    if (m_starsStatusLabel != nullptr)
        m_starsStatusLabel->setText(QStringLiteral("%1: stars.json").arg(prefix));
}

void MainWindow::updateBodyColumnGuide() {
    const QStringList lines = m_bodyEdit->toPlainText().split(QLatin1Char('\n'));
    m_bodyEdit->setColumnGuide(effectiveBodyLineLimit(lines));
}

void MainWindow::configureCrawlWindow(CrawlWindow *window) {
    connect(window, &CrawlWindow::destroyed, this, [this]() {
        m_crawlWindow = nullptr;
    });
    connect(window, &CrawlWindow::closed, this, [this]() {
        if (m_crawlWindow != nullptr && !m_crawlWindow->isFullScreen()) {
            setGeometry(m_crawlWindow->geometry());
        }
        showNormal();
        raise();
        activateWindow();
        m_pages->setCurrentWidget(m_launcherPage);
        refreshModeUi();
        updateStatusLabels();
    });
}

void MainWindow::ensureCrawlWindow() {
    if (m_crawlWindow != nullptr)
        return;
    m_crawlWindow = new CrawlWindow();
    configureCrawlWindow(m_crawlWindow);
}

bool MainWindow::saveEditorContents() {
    QString textSavedPath;
    if (!saveRawText(serializeContent(collectContent()), &textSavedPath)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Save failed"),
            QStringLiteral("The text file could not be saved. The embedded Qt resource is read-only at runtime."));
        return false;
    }

    QString starsSavedPath;
    if (!saveRawStars(serializeStars(m_starsEditor->stars()), &starsSavedPath)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Save failed"),
            QStringLiteral("The star configuration file could not be saved. The embedded Qt resource is read-only at runtime."));
        return false;
    }
    m_textFormModified = false;
    m_starsEditor->setModified(false);
    m_hasUnsavedChanges = false;
    updateStatusLabels();
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

void MainWindow::openEditor(const int tabIndex) {
    m_pages->setCurrentWidget(tabIndex == 0 ? m_textEditorPage : m_starsEditorPage);
    updateStatusLabels();
}

bool MainWindow::leaveEditor() {
    const QWidget *current = m_pages->currentWidget();
    if (current != m_textEditorPage && current != m_starsEditorPage)
        return true;
    if (!confirmDiscardOrSave())
        return false;
    m_pages->setCurrentWidget(m_launcherPage);
    return true;
}

void MainWindow::enterShowMode() {
    ensureCrawlWindow();
    m_crawlWindow->setContent(collectContent());
    m_crawlWindow->setGoalStars(m_starsEditor->stars());
    m_crawlWindow->setShowMode(
        m_startupMode == StartupMode::Live
            ? CrawlWindow::ShowMode::Live
            : CrawlWindow::ShowMode::VideoGame);
    m_crawlWindow->setHyperspaceMode(
        m_hyperspaceMode == 0
            ? CrawlWindow::HyperspaceMode::Tunnel
            : CrawlWindow::HyperspaceMode::Particles);
    hide();
    m_crawlWindow->openShowWindow(m_startFullscreen, m_startFullscreen ? QSize() : size());
    updateStatusLabels();
}

void MainWindow::refreshModeUi() {
    if (m_liveAction != nullptr)       m_liveAction->setChecked(m_startupMode == StartupMode::Live);
    if (m_videoGameAction != nullptr)  m_videoGameAction->setChecked(m_startupMode == StartupMode::VideoGame);
    if (m_windowedAction != nullptr)   m_windowedAction->setChecked(!m_startFullscreen);
    if (m_fullscreenAction != nullptr) m_fullscreenAction->setChecked(m_startFullscreen);

    if (m_liveCard != nullptr)         m_liveCard->setChecked(m_startupMode == StartupMode::Live);
    if (m_videoGameCard != nullptr)    m_videoGameCard->setChecked(m_startupMode == StartupMode::VideoGame);
    if (m_windowedCard != nullptr)     m_windowedCard->setChecked(!m_startFullscreen);
    if (m_fullscreenCard != nullptr)   m_fullscreenCard->setChecked(m_startFullscreen);
    if (m_tunnelAction != nullptr)     m_tunnelAction->setChecked(m_hyperspaceMode == 0);
    if (m_particlesAction != nullptr)  m_particlesAction->setChecked(m_hyperspaceMode == 1);
    if (m_tunnelCard != nullptr)       m_tunnelCard->setChecked(m_hyperspaceMode == 0);
    if (m_particlesCard != nullptr)    m_particlesCard->setChecked(m_hyperspaceMode == 1);
}
