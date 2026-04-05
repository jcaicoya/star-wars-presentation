#include "TextIO.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

QString resourceTextPath() {
    return QStringLiteral(":/text.txt");
}

QString resourceStarsPath() {
    return QStringLiteral(":/stars.json");
}

static QStringList candidateTextPaths() {
    const QString currentPath  = QDir::cleanPath(QDir::current().filePath(QStringLiteral("resources/text.txt")));
    const QString appDir       = QCoreApplication::applicationDirPath();
    const QString nearBinary   = QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../resources/text.txt")));
    const QString aboveBinary  = QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../../resources/text.txt")));
    return {currentPath, nearBinary, aboveBinary};
}

static QStringList candidateStarsPaths() {
    const QString currentPath  = QDir::cleanPath(QDir::current().filePath(QStringLiteral("resources/stars.json")));
    const QString appDir       = QCoreApplication::applicationDirPath();
    const QString nearBinary   = QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../resources/stars.json")));
    const QString aboveBinary  = QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../../resources/stars.json")));
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

QString editableStarsPath() {
    const QStringList candidates = candidateStarsPaths();
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

QString loadRawStars() {
    const QString diskPath = editableStarsPath();
    if (!diskPath.isEmpty()) {
        QFile diskFile(diskPath);
        if (diskFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString::fromUtf8(diskFile.readAll());
    }

    QFile resourceFile(resourceStarsPath());
    if (resourceFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString::fromUtf8(resourceFile.readAll());

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

bool saveRawStars(const QString &text, QString *savedPath) {
    const QString path = editableStarsPath();
    if (path.isEmpty())
        return false;

    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    file.write(text.toUtf8());
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

std::vector<StarDefinition> parseStars(const QString &rawText) {
    std::vector<StarDefinition> stars;

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return stars;

    const QJsonArray starArray = document.object().value(QStringLiteral("stars")).toArray();
    stars.reserve(starArray.size());

    for (const QJsonValue &value : starArray) {
        if (!value.isObject())
            continue;

        const QJsonObject object = value.toObject();
        const QJsonObject position = object.value(QStringLiteral("position")).toObject();
        const QJsonObject colors   = object.value(QStringLiteral("colors")).toObject();

        StarDefinition star;
        star.text = object.value(QStringLiteral("text")).toString();
        star.position = QVector3D(
            static_cast<float>(position.value(QStringLiteral("x")).toDouble()),
            static_cast<float>(position.value(QStringLiteral("y")).toDouble()),
            static_cast<float>(position.value(QStringLiteral("z")).toDouble()));
        star.coreColor = QColor(colors.value(QStringLiteral("core")).toString(QStringLiteral("#ffffff")));
        star.glowColor = QColor(colors.value(QStringLiteral("glow")).toString(QStringLiteral("#99ccff")));
        star.radius = object.value(QStringLiteral("radius")).toDouble(7.0);

        if (!star.coreColor.isValid())
            star.coreColor = QColor(255, 255, 255);
        if (!star.glowColor.isValid())
            star.glowColor = QColor(153, 204, 255);

        stars.push_back(star);
    }

    return stars;
}
