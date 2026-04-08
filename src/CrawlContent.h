#pragma once

#include <QColor>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVector3D>

struct CrawlContent {
    QString     intro;
    QString     logo;
    QString     title;
    QString     subtitle;
    QStringList bodyLines;
    QString     planetHeader;
    QString     finalSentence;
};

struct Star {
    QPointF position;
    qreal radius = 1.0;
    qreal depth  = 0.5;
    qreal twinklePhase = 0.0;
    QColor color;
};

struct StarDefinition {
    QString   text;
    QVector3D position;
    QColor    coreColor;
    QColor    glowColor;
    qreal     radius = 7.0;
    bool      isPlanet = false;
};
