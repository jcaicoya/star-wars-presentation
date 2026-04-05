#pragma once

#include <QColor>
#include <QPointF>
#include <QString>
#include <QStringList>

struct CrawlContent {
    QString     intro;
    QString     logo;
    QString     title;
    QString     subtitle;
    QStringList bodyLines;
};

struct Star {
    QPointF position;
    qreal radius = 1.0;
    QColor color;
};
