#include "datamanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>

QVector<MediaItem> DataManager::loadItems(const QString &filePath)
{
    QVector<MediaItem> items;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return items;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        items.append(MediaItem::fromJson(val.toObject()));
    }
    return items;
}

void DataManager::saveItems(const QString &filePath, const QVector<MediaItem> &items)
{
    QJsonArray arr;
    for (const MediaItem &item : items)
        arr.append(item.toJson());

    QJsonDocument doc(arr);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson());
    }
}

QVector<UpcomingItem> DataManager::loadUpcomingItems(const QString &filePath)
{
    QVector<UpcomingItem> items;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return items;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        items.append(UpcomingItem::fromJson(val.toObject()));
    }
    return items;
}

void DataManager::saveUpcomingItems(const QString &filePath, const QVector<UpcomingItem> &items)
{
    QJsonArray arr;
    for (const UpcomingItem &item : items)
        arr.append(item.toJson());

    QJsonDocument doc(arr);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson());
    }
}
