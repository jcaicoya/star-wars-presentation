#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFont>
#include <QFontMetrics>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QScreen>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <functional>
#include <vector>

namespace {

struct CrawlContent {
    QString title;
    QString subtitle;
    QStringList bodyLines;
};

struct Star {
    QPointF position;
    qreal radius = 1.0;
    QColor color;
};

QString resourceTextPath() {
    return QStringLiteral(":/text.txt");
}

QStringList candidateTextPaths() {
    const QString currentPath = QDir::cleanPath(QDir::current().filePath(QStringLiteral("resources/text.txt")));
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString nearBinary = QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../resources/text.txt")));
    const QString aboveBinary = QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../../resources/text.txt")));

    return {currentPath, nearBinary, aboveBinary};
}

QString editableTextPath() {
    const QStringList candidates = candidateTextPaths();
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return candidates.isEmpty() ? QString() : candidates.first();
}

QString loadRawText() {
    const QString diskPath = editableTextPath();
    if (!diskPath.isEmpty()) {
        QFile diskFile(diskPath);
        if (diskFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&diskFile);
            return stream.readAll();
        }
    }

    QFile resourceFile(resourceTextPath());
    if (resourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&resourceFile);
        return stream.readAll();
    }

    return QString();
}

bool saveRawText(const QString &text, QString *savedPath) {
    const QString path = editableTextPath();
    if (path.isEmpty()) {
        return false;
    }

    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    QTextStream stream(&file);
    stream << text;
    if (savedPath != nullptr) {
        *savedPath = path;
    }
    return true;
}

CrawlContent parseContent(const QString &rawText) {
    QString normalized = rawText;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QChar::CarriageReturn, QChar::LineFeed);

    const QStringList lines = normalized.split(QChar::LineFeed, Qt::KeepEmptyParts);

    CrawlContent content;
    if (!lines.isEmpty()) {
        content.title = lines.at(0).trimmed();
    }
    if (lines.size() > 1) {
        content.subtitle = lines.at(1).trimmed();
    }
    for (int i = 2; i < lines.size(); ++i) {
        content.bodyLines.append(lines.at(i));
    }
    return content;
}

QString consoleBodyText(const CrawlContent &content) {
    QStringList output;
    output << QStringLiteral("> title: %1").arg(content.title.isEmpty() ? QStringLiteral("(empty)") : content.title);
    output << QStringLiteral("> subtitle: %1").arg(content.subtitle.isEmpty() ? QStringLiteral("(empty)") : content.subtitle);
    output << QStringLiteral(">");

    if (content.bodyLines.isEmpty()) {
        output << QStringLiteral("> no body lines loaded");
    } else {
        for (const QString &line : content.bodyLines) {
            output << QStringLiteral("> %1").arg(line);
        }
    }

    return output.join(QChar::LineFeed);
}

class CrawlWindow final : public QWidget {
public:
    explicit CrawlWindow(QWidget *parent = nullptr)
        : QWidget(parent) {
        setWindowTitle(QStringLiteral("StarWarsText - Show"));
        setAttribute(Qt::WA_DeleteOnClose, false);
        setAutoFillBackground(false);
        regenerateStars();

        m_animationTimer.setInterval(16);
        connect(&m_animationTimer, &QTimer::timeout, this, [this]() {
            m_offset -= 1.4;
            update();
        });
    }

    void setContent(const CrawlContent &content) {
        m_content = content;
        rebuildLines();
    }

    void openFullscreen() {
        rebuildLines();
        showFullScreen();
        m_offset = static_cast<qreal>(height()) + 40.0;
        raise();
        activateWindow();
        m_animationTimer.start();
        update();
    }

    std::function<void()> onClosed;

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.fillRect(rect(), QColor(0, 0, 0));
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        painter.setPen(Qt::NoPen);
        for (const Star &star : m_stars) {
            painter.setBrush(star.color);
            painter.drawEllipse(star.position, star.radius, star.radius);
        }

        painter.setPen(QColor(255, 230, 120));

        qreal y = m_offset;
        for (int i = 0; i < m_renderLines.size(); ++i) {
            const RenderLine &line = m_renderLines.at(i);
            if (y + line.height < 0) {
                y += line.height + line.spacingAfter;
                continue;
            }
            if (y > height()) {
                break;
            }

            painter.setFont(line.font);
            painter.drawText(QRectF(0.0, y, width(), line.height), Qt::AlignHCenter | Qt::AlignVCenter, line.text);
            y += line.height + line.spacingAfter;
        }

        painter.setPen(QColor(180, 180, 180));
        painter.setFont(QFont(QStringLiteral("Consolas"), 11));
        painter.drawText(QRectF(20.0, height() - 36.0, width() - 40.0, 20.0),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("Esc to exit show mode"));
    }

    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        regenerateStars();
        rebuildLines();
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape) {
            close();
            return;
        }
        QWidget::keyPressEvent(event);
    }

    void closeEvent(QCloseEvent *event) override {
        m_animationTimer.stop();
        if (onClosed) {
            onClosed();
        }
        QWidget::closeEvent(event);
    }

private:
    struct RenderLine {
        QString text;
        QFont font;
        qreal height = 0.0;
        qreal spacingAfter = 0.0;
    };

    void regenerateStars() {
        m_stars.clear();

        const int safeWidth = std::max(1, width());
        const int safeHeight = std::max(1, height());
        const int starCount = std::max(120, (safeWidth * safeHeight) / 9000);
        auto *random = QRandomGenerator::global();

        for (int i = 0; i < starCount; ++i) {
            Star star;
            star.position = QPointF(random->bounded(safeWidth), random->bounded(safeHeight));
            star.radius = 0.6 + (random->bounded(160) / 100.0);

            const int brightness = 170 + random->bounded(86);
            const int blueTint = 200 + random->bounded(56);
            star.color = QColor(brightness, brightness, blueTint, 220);
            m_stars.push_back(star);
        }
    }

    void rebuildLines() {
        m_renderLines.clear();

        const int titleSize = std::max(28, height() / 12);
        const int subtitleSize = std::max(20, height() / 18);
        const int bodySize = std::max(18, height() / 24);

        QFont titleFont(QStringLiteral("Arial"), titleSize, QFont::Bold);
        QFont subtitleFont(QStringLiteral("Arial"), subtitleSize, QFont::DemiBold);
        QFont bodyFont(QStringLiteral("Consolas"), bodySize);

        if (!m_content.title.trimmed().isEmpty()) {
            appendLine(m_content.title, titleFont, 18.0);
        }

        if (!m_content.subtitle.trimmed().isEmpty()) {
            appendLine(m_content.subtitle, subtitleFont, 34.0);
        }

        for (const QString &line : m_content.bodyLines) {
            if (line.trimmed().isEmpty()) {
                appendLine(QString(), bodyFont, 18.0);
            } else {
                appendWrappedLines(line, bodyFont, 10.0);
            }
        }

        m_offset = static_cast<qreal>(height()) + 40.0;
    }

    void appendLine(const QString &text, const QFont &font, qreal spacingAfter) {
        const QFontMetrics metrics(font);
        RenderLine line;
        line.text = text;
        line.font = font;
        line.height = std::max(metrics.height() * 1.2, 24.0);
        line.spacingAfter = spacingAfter;
        m_renderLines.push_back(line);
    }

    void appendWrappedLines(const QString &text, const QFont &font, qreal spacingAfter) {
        const int maxWidth = std::max(200, static_cast<int>(width() * 0.72));
        const QFontMetrics metrics(font);
        const QStringList words = text.split(' ', Qt::SkipEmptyParts);

        QString currentLine;
        for (const QString &word : words) {
            const QString candidate = currentLine.isEmpty() ? word : currentLine + QStringLiteral(" ") + word;
            if (!currentLine.isEmpty() && metrics.horizontalAdvance(candidate) > maxWidth) {
                appendLine(currentLine, font, spacingAfter);
                currentLine = word;
            } else {
                currentLine = candidate;
            }
        }

        if (!currentLine.isEmpty()) {
            appendLine(currentLine, font, spacingAfter);
        }

        if (words.isEmpty()) {
            appendLine(QString(), font, spacingAfter);
        }
    }

private:
    CrawlContent m_content;
    std::vector<Star> m_stars;
    std::vector<RenderLine> m_renderLines;
    QTimer m_animationTimer;
    qreal m_offset = 0.0;
};

class MainWindow final : public QMainWindow {
public:
    MainWindow() {
        setWindowTitle(QStringLiteral("StarWarsText"));
        resize(1000, 720);

        auto *central = new QWidget(this);
        auto *rootLayout = new QVBoxLayout(central);
        rootLayout->setContentsMargins(16, 16, 16, 16);
        rootLayout->setSpacing(12);

        auto *toolbarLayout = new QHBoxLayout();
        toolbarLayout->setSpacing(8);

        auto *editButton = new QPushButton(QStringLiteral("Edit"), this);
        auto *showButton = new QPushButton(QStringLiteral("Show"), this);
        auto *reloadButton = new QPushButton(QStringLiteral("Reload"), this);
        auto *saveButton = new QPushButton(QStringLiteral("Save"), this);

        m_statusLabel = new QLabel(this);
        m_statusLabel->setMinimumWidth(320);

        toolbarLayout->addWidget(editButton);
        toolbarLayout->addWidget(showButton);
        toolbarLayout->addWidget(reloadButton);
        toolbarLayout->addWidget(saveButton);
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

        m_crawlWindow = new CrawlWindow();
        connect(m_crawlWindow, &CrawlWindow::destroyed, this, [this]() {
            m_crawlWindow = nullptr;
        });
        m_crawlWindow->onClosed = [this]() {
            this->showNormal();
            this->raise();
            this->activateWindow();
            setStatusForCurrentPath(QStringLiteral("Editing"));
        };

        connect(editButton, &QPushButton::clicked, this, [this]() {
            setStatusForCurrentPath(QStringLiteral("Editing"));
        });

        connect(showButton, &QPushButton::clicked, this, [this]() {
            if (m_crawlWindow == nullptr) {
                m_crawlWindow = new CrawlWindow();
                connect(m_crawlWindow, &CrawlWindow::destroyed, this, [this]() {
                    m_crawlWindow = nullptr;
                });
                m_crawlWindow->onClosed = [this]() {
                    this->showNormal();
                    this->raise();
                    this->activateWindow();
                    setStatusForCurrentPath(QStringLiteral("Editing"));
                };
            }

            m_crawlWindow->setContent(parseContent(m_editor->toPlainText()));
            hide();
            m_crawlWindow->openFullscreen();
            setStatusForCurrentPath(QStringLiteral("Showing"));
        });

        connect(reloadButton, &QPushButton::clicked, this, [this]() {
            loadIntoEditor();
            setStatusForCurrentPath(QStringLiteral("Reloaded"));
        });

        connect(saveButton, &QPushButton::clicked, this, [this]() {
            QString savedPath;
            if (!saveRawText(m_editor->toPlainText(), &savedPath)) {
                QMessageBox::warning(
                    this,
                    QStringLiteral("Save failed"),
                    QStringLiteral("The file could not be saved. The embedded Qt resource is read-only at runtime.")
                );
                return;
            }

            setStatusForCurrentPath(QStringLiteral("Saved"), savedPath);
        });

        loadIntoEditor();
        setStatusForCurrentPath(QStringLiteral("Loaded"));
    }

private:
    void loadIntoEditor() {
        m_editor->setPlainText(loadRawText());
    }

    void setStatusForCurrentPath(const QString &prefix, const QString &overridePath = QString()) {
        const QString path = overridePath.isEmpty() ? editableTextPath() : overridePath;
        m_statusLabel->setText(QStringLiteral("%1: %2").arg(prefix, path.isEmpty() ? resourceTextPath() : path));
    }

    QPlainTextEdit *m_editor = nullptr;
    QLabel *m_statusLabel = nullptr;
    CrawlWindow *m_crawlWindow = nullptr;
};

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
