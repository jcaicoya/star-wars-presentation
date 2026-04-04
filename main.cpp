#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFont>
#include <QFontMetrics>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QLinearGradient>
#include <QMainWindow>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPolygonF>
#include <QPushButton>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QScreen>
#include <QShortcut>
#include <QTextStream>
#include <QTimer>
#include <QTransform>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
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
    int index = 0;

    while (index < lines.size() && lines.at(index).trimmed().isEmpty()) {
        ++index;
    }
    if (index < lines.size()) {
        content.title = lines.at(index).trimmed();
        ++index;
    }

    while (index < lines.size() && lines.at(index).trimmed().isEmpty()) {
        ++index;
    }
    if (index < lines.size()) {
        content.subtitle = lines.at(index).trimmed();
        ++index;
    }

    for (; index < lines.size(); ++index) {
        content.bodyLines.append(lines.at(index));
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
            m_offset += 1.1;
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
        m_offset = 0.0;
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

        if (!m_crawlImage.isNull()) {
            const QRectF viewport = crawlViewport();
            const qreal topScreenY = viewport.top() + (viewport.height() * 0.14);
            const qreal bottomScreenY = viewport.bottom() + (viewport.height() * 0.02);
            const qreal topWidth = viewport.width() * 0.08;
            const qreal bottomWidth = viewport.width() * 0.92;
            const qreal topShiftX = viewport.width() * 0.025;
            const qreal sourceTop = m_offset;
            const qreal sourceBottom = m_offset + m_sourceWindowHeight;

            QPolygonF sourceQuad;
            sourceQuad << QPointF(0.0, sourceTop)
                       << QPointF(m_crawlImage.width(), sourceTop)
                       << QPointF(m_crawlImage.width(), sourceBottom)
                       << QPointF(0.0, sourceBottom);

            QPolygonF destinationQuad;
            destinationQuad << QPointF(viewport.center().x() - (topWidth * 0.5) + topShiftX, topScreenY)
                            << QPointF(viewport.center().x() + (topWidth * 0.5) + topShiftX, topScreenY)
                            << QPointF(viewport.center().x() + (bottomWidth * 0.5), bottomScreenY)
                            << QPointF(viewport.center().x() - (bottomWidth * 0.5), bottomScreenY);

            QTransform transform;
            if (QTransform::quadToQuad(sourceQuad, destinationQuad, transform)) {
                painter.setWorldTransform(transform, true);
                painter.drawImage(QPointF(0.0, 0.0), m_crawlImage);
                painter.resetTransform();
            }
        }

        const QRectF viewport = crawlViewport();
        const qreal horizonY = viewport.top() + (viewport.height() * 0.14);
        QLinearGradient fadeGradient(0.0, horizonY - 30.0, 0.0, viewport.top() + viewport.height() * 0.42);
        fadeGradient.setColorAt(0.0, QColor(0, 0, 0, 255));
        fadeGradient.setColorAt(0.45, QColor(0, 0, 0, 150));
        fadeGradient.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(QRectF(viewport.left(), viewport.top(), viewport.width(), viewport.height() * 0.45), fadeGradient);

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
    enum class LineAlignment {
        Center,
        Left
    };

    struct RenderLine {
        QString text;
        QFont font;
        LineAlignment alignment = LineAlignment::Left;
        qreal height = 0.0;
        qreal advance = 0.0;
        qreal spacingAfter = 0.0;
    };

    QRectF crawlViewport() const {
        const qreal marginX = width() * 0.08;
        const qreal marginY = height() * 0.06;
        const qreal availableWidth = std::max(320.0, width() - (marginX * 2.0));
        const qreal availableHeight = std::max(240.0, height() - (marginY * 2.0));
        const qreal targetAspect = 16.0 / 9.0;

        qreal viewportWidth = availableWidth;
        qreal viewportHeight = viewportWidth / targetAspect;
        if (viewportHeight > availableHeight) {
            viewportHeight = availableHeight;
            viewportWidth = viewportHeight * targetAspect;
        }

        return QRectF((width() - viewportWidth) * 0.5,
                      (height() - viewportHeight) * 0.5,
                      viewportWidth,
                      viewportHeight);
    }

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

        const QRectF viewport = crawlViewport();

        const int titleSize = std::max(16, static_cast<int>(viewport.height() / 18.0));
        const int subtitleSize = std::max(16, static_cast<int>(viewport.height() / 20.0));
        const int bodySize = std::max(14, static_cast<int>(viewport.height() / 27.0));

        QFont titleFont(QStringLiteral("Arial"), titleSize, QFont::Bold);
        QFont subtitleFont(QStringLiteral("Arial"), subtitleSize, QFont::DemiBold);
        QFont bodyFont(QStringLiteral("Consolas"), bodySize);

        if (!m_content.title.trimmed().isEmpty()) {
            appendLine(m_content.title, titleFont, LineAlignment::Center, viewport.height() * 0.025);
        }

        if (!m_content.subtitle.trimmed().isEmpty()) {
            appendLine(m_content.subtitle, subtitleFont, LineAlignment::Center, viewport.height() * 0.018);
        }

        for (const QString &line : m_content.bodyLines) {
            if (line.trimmed().isEmpty()) {
                appendLine(QString(), bodyFont, LineAlignment::Left, viewport.height() * 0.004);
            } else {
                appendLine(line, bodyFont, LineAlignment::Left, viewport.height() * 0.0015);
            }
        }

        renderCrawlImage();
        m_offset = 0.0;
    }

    void appendLine(const QString &text, const QFont &font, LineAlignment alignment, qreal spacingAfter) {
        const QFontMetricsF metrics(font);
        RenderLine line;
        line.text = text;
        line.font = font;
        line.alignment = alignment;
        line.height = std::max(metrics.height() * 1.0, 12.0);
        line.advance = std::max(metrics.height() * 0.92, 10.0);
        line.spacingAfter = spacingAfter;
        m_renderLines.push_back(line);
    }

    void renderCrawlImage() {
        const QRectF viewport = crawlViewport();
        QFont bodyFont(QStringLiteral("Consolas"), std::max(14, static_cast<int>(viewport.height() / 27.0)));
        const QFontMetricsF bodyMetrics(bodyFont);
        const qreal bodyColumnWidth = bodyMetrics.horizontalAdvance(QStringLiteral("X")) * 30.0;
        m_sourceWindowHeight = std::max(860.0, viewport.height() * 3.35);
        const qreal entryPadding = m_sourceWindowHeight * 1.0;
        const qreal exitPadding = m_sourceWindowHeight * 0.45;
        const int imageWidth = std::max(320, static_cast<int>(std::ceil(bodyColumnWidth + 80.0)));
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
            const Qt::Alignment horizontalAlignment =
                line.alignment == LineAlignment::Center ? Qt::AlignHCenter : Qt::AlignLeft;

            imagePainter.setPen(QColor(255, 215, 90, 48));
            imagePainter.drawText(lineRect.translated(-1.0, 0.0),
                                  horizontalAlignment | Qt::AlignVCenter,
                                  line.text);
            imagePainter.drawText(lineRect.translated(1.0, 0.0),
                                  horizontalAlignment | Qt::AlignVCenter,
                                  line.text);
            imagePainter.drawText(lineRect.translated(0.0, -1.0),
                                  horizontalAlignment | Qt::AlignVCenter,
                                  line.text);
            imagePainter.drawText(lineRect.translated(0.0, 1.0),
                                  horizontalAlignment | Qt::AlignVCenter,
                                  line.text);

            imagePainter.setPen(QColor(255, 228, 110));
            imagePainter.drawText(lineRect,
                                  horizontalAlignment | Qt::AlignVCenter,
                                  line.text);
            y += line.advance + line.spacingAfter;
        }
    }

    qreal totalContentHeight() const {
        qreal total = 0.0;
        for (const RenderLine &line : m_renderLines) {
            total += line.advance + line.spacingAfter;
        }
        return total;
    }

private:
    CrawlContent m_content;
    std::vector<Star> m_stars;
    std::vector<RenderLine> m_renderLines;
    QImage m_crawlImage;
    qreal m_sourceWindowHeight = 0.0;
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

        connect(showButton, &QPushButton::clicked, this, [this]() {
            enterShowMode();
        });

        connect(m_editor->document(), &QTextDocument::modificationChanged, this, [this](bool modified) {
            m_hasUnsavedChanges = modified;
            setStatusForCurrentPath(modified ? QStringLiteral("Modified") : QStringLiteral("Editing"));
        });

        auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
        connect(saveShortcut, &QShortcut::activated, this, [this]() {
            saveEditorContents();
        });

        loadIntoEditor();
        setStatusForCurrentPath(QStringLiteral("Loaded"));
        QTimer::singleShot(0, this, [this]() {
            enterShowMode();
        });
    }

private:
    void closeEvent(QCloseEvent *event) override {
        if (!confirmDiscardOrSave()) {
            event->ignore();
            return;
        }

        if (m_crawlWindow != nullptr) {
            m_crawlWindow->close();
        }

        QMainWindow::closeEvent(event);
    }

    void loadIntoEditor() {
        m_editor->setPlainText(loadRawText());
        m_editor->document()->setModified(false);
        m_hasUnsavedChanges = false;
    }

    void setStatusForCurrentPath(const QString &prefix, const QString &overridePath = QString()) {
        const QString path = overridePath.isEmpty() ? editableTextPath() : overridePath;
        m_statusLabel->setText(QStringLiteral("%1: %2").arg(prefix, path.isEmpty() ? resourceTextPath() : path));
    }

    void ensureCrawlWindow() {
        if (m_crawlWindow != nullptr) {
            return;
        }

        m_crawlWindow = new CrawlWindow();
        connect(m_crawlWindow, &CrawlWindow::destroyed, this, [this]() {
            m_crawlWindow = nullptr;
        });
        m_crawlWindow->onClosed = [this]() {
            this->showNormal();
            this->raise();
            this->activateWindow();
            setStatusForCurrentPath(m_hasUnsavedChanges ? QStringLiteral("Modified") : QStringLiteral("Editing"));
        };
    }

    bool saveEditorContents() {
        QString savedPath;
        if (!saveRawText(m_editor->toPlainText(), &savedPath)) {
            QMessageBox::warning(
                this,
                QStringLiteral("Save failed"),
                QStringLiteral("The file could not be saved. The embedded Qt resource is read-only at runtime.")
            );
            return false;
        }

        m_editor->document()->setModified(false);
        m_hasUnsavedChanges = false;
        setStatusForCurrentPath(QStringLiteral("Saved"), savedPath);
        return true;
    }

    bool confirmDiscardOrSave() {
        if (!m_hasUnsavedChanges) {
            return true;
        }

        const QMessageBox::StandardButton choice = QMessageBox::question(
            this,
            QStringLiteral("Unsaved changes"),
            QStringLiteral("There are unsaved changes. Do you want to save them?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save
        );

        if (choice == QMessageBox::Cancel) {
            return false;
        }
        if (choice == QMessageBox::Save) {
            return saveEditorContents();
        }
        return true;
    }

    void enterShowMode() {
        ensureCrawlWindow();
        m_crawlWindow->setContent(parseContent(m_editor->toPlainText()));
        hide();
        m_crawlWindow->openFullscreen();
        setStatusForCurrentPath(QStringLiteral("Showing"));
    }

    QPlainTextEdit *m_editor = nullptr;
    QLabel *m_statusLabel = nullptr;
    CrawlWindow *m_crawlWindow = nullptr;
    bool m_hasUnsavedChanges = false;
};

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
