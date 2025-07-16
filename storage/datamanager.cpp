#include "datamanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>

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

QString getAppDataPath() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path + "/data");
    return path;
}

QStringList DataManager::loadWatchedItems()
{
    QString filePath = getAppDataPath() + "/data/watched.json";
    QStringList items;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return items;

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        items << val.toString();
    }
    return items;
}

void DataManager::saveWatchedItems(const QStringList &items)
{
    QString filePath = getAppDataPath() + "/data/watched.json";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return;

    QJsonArray arr;
    for (const QString &item : items) {
        arr.append(item);
    }

    QJsonDocument doc(arr);
    file.write(doc.toJson());
}
