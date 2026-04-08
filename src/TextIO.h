#pragma once

#include <vector>
#include <QString>
#include "CrawlContent.h"

QString resourceTextPath();
QString editableTextPath();
QString resourceStarsPath();
QString loadRawText();
QString loadRawStars();
bool    saveRawText(const QString &text, QString *savedPath = nullptr);
bool    saveRawStars(const QString &text, QString *savedPath = nullptr);
CrawlContent parseContent(const QString &rawText);
QString serializeContent(const CrawlContent &content);
std::vector<StarDefinition> parseStars(const QString &rawText);
QString serializeStars(const std::vector<StarDefinition> &stars);
