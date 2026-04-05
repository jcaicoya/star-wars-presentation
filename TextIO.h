#pragma once

#include <QString>
#include "CrawlContent.h"

QString resourceTextPath();
QString editableTextPath();
QString loadRawText();
bool    saveRawText(const QString &text, QString *savedPath = nullptr);
CrawlContent parseContent(const QString &rawText);
