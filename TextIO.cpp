#include "TextIO.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

QString resourceTextPath() {
    return QStringLiteral(":/text.txt");
}

static QStringList candidateTextPaths() {
    const QString currentPath  = QDir::cleanPath(QDir::current().filePath(QStringLiteral("resources/text.txt")));
    const QString appDir       = QCoreApplication::applicationDirPath();
    const QString nearBinary   = QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../resources/text.txt")));
    const QString aboveBinary  = QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../../resources/text.txt")));
    return {currentPath, nearBinary, aboveBinary};
}

QString editableTextPath() {
    const QStringList candidates = candidateTextPaths();
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path))
            return path;
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
    if (path.isEmpty())
        return false;

    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    QTextStream stream(&file);
    stream << text;
    if (savedPath != nullptr)
        *savedPath = path;
    return true;
}

// Parses a section-based text file.
// Each [section] header introduces a block that runs until the next header or EOF.
// Leading/trailing blank lines within each block are stripped.
// Unknown section names are silently ignored.
CrawlContent parseContent(const QString &rawText) {
    QString normalized = rawText;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QChar::CarriageReturn, QChar::LineFeed);

    const QStringList lines = normalized.split(QChar::LineFeed, Qt::KeepEmptyParts);

    // Collect raw lines per section name, preserving order of appearance
    QMap<QString, QStringList> sections;
    QString currentSection;

    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QLatin1Char('[')) && trimmed.endsWith(QLatin1Char(']'))) {
            currentSection = trimmed.mid(1, trimmed.size() - 2).toLower();
        } else if (!currentSection.isEmpty()) {
            sections[currentSection].append(line);
        }
    }

    // Strip leading and trailing blank lines from a section's line list
    auto trim = [](QStringList block) -> QStringList {
        while (!block.isEmpty() && block.first().trimmed().isEmpty())
            block.removeFirst();
        while (!block.isEmpty() && block.last().trimmed().isEmpty())
            block.removeLast();
        return block;
    };

    CrawlContent content;

    if (sections.contains(QStringLiteral("intro")))
        content.intro = trim(sections[QStringLiteral("intro")]).join(QLatin1Char('\n'));

    if (sections.contains(QStringLiteral("logo")))
        content.logo = trim(sections[QStringLiteral("logo")]).join(QLatin1Char('\n'));

    if (sections.contains(QStringLiteral("title")))
        content.title = trim(sections[QStringLiteral("title")]).join(QLatin1Char('\n'));

    if (sections.contains(QStringLiteral("subtitle")))
        content.subtitle = trim(sections[QStringLiteral("subtitle")]).join(QLatin1Char('\n'));

    if (sections.contains(QStringLiteral("body")))
        content.bodyLines = sections[QStringLiteral("body")];

    return content;
}
