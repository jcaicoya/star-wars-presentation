#include "MainWindow.h"
#include "CrawlWindow.h"
#include "TextIO.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

// ── Construction ──────────────────────────────────────────────────────────────

MainWindow::MainWindow() {
    setWindowTitle(QStringLiteral("StarWarsText"));
    resize(1000, 720);

    auto *central    = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    auto *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    auto *showButton = new QPushButton(QStringLiteral("Show"), this);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setMinimumWidth(320);

    toolbarLayout->addWidget(showButton);
    toolbarLayout->addStretch(1);
    toolbarLayout->addWidget(m_statusLabel);

    m_editor = new QPlainTextEdit(this);
    m_editor->setStyleSheet(
        "QPlainTextEdit {"
        "background: #161616;"
        "color: #f1f1f1;"
        "border: 1px solid #303030;"
        "font: 16px 'Consolas';"
        "padding: 12px;"
        "selection-background-color: #404040;"
        "}"
    );

    rootLayout->addLayout(toolbarLayout);
    rootLayout->addWidget(m_editor, 1);
    setCentralWidget(central);

    ensureCrawlWindow();

    connect(showButton, &QPushButton::clicked, this, [this]() {
        enterShowMode();
    });

    connect(m_editor->document(), &QTextDocument::modificationChanged, this, [this](bool modified) {
        m_hasUnsavedChanges = modified;
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
    setStatusForCurrentPath(QStringLiteral("Loaded"));
    QTimer::singleShot(0, this, [this]() { enterShowMode(); });
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

void MainWindow::loadIntoEditor() {
    m_editor->setPlainText(loadRawText());
    m_editor->document()->setModified(false);
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
    QString savedPath;
    if (!saveRawText(m_editor->toPlainText(), &savedPath)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Save failed"),
            QStringLiteral("The file could not be saved. The embedded Qt resource is read-only at runtime."));
        return false;
    }
    m_editor->document()->setModified(false);
    m_hasUnsavedChanges = false;
    setStatusForCurrentPath(QStringLiteral("Saved"), savedPath);
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
    hide();
    m_crawlWindow->openShowWindow();
    setStatusForCurrentPath(QStringLiteral("Showing"));
}
