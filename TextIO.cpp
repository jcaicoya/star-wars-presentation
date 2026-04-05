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

CrawlContent parseContent(const QString &rawText) {
    QString normalized = rawText;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QChar::CarriageReturn, QChar::LineFeed);

    const QStringList lines = normalized.split(QChar::LineFeed, Qt::KeepEmptyParts);

    CrawlContent content;
    int index = 0;

    while (index < lines.size() && lines.at(index).trimmed().isEmpty())
        ++index;
    if (index < lines.size()) {
        content.title = lines.at(index).trimmed();
        ++index;
    }

    while (index < lines.size() && lines.at(index).trimmed().isEmpty())
        ++index;
    if (index < lines.size()) {
        content.subtitle = lines.at(index).trimmed();
        ++index;
    }

    for (; index < lines.size(); ++index)
        content.bodyLines.append(lines.at(index));

    return content;
}
