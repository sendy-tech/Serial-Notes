#include "datamanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

QVector<MediaItem> DataManager::loadItems(const QString &filePath)
{
    QVector<MediaItem> items;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return items;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.array();

    for (const auto &val : array)
        items.append(MediaItem(val.toObject()));

    return items;
}

void DataManager::saveItems(const QString &filePath, const QVector<MediaItem> &items)
{
    QJsonArray array;
    for (const auto &item : items)
        array.append(item.toJson());

    QJsonDocument doc(array);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}
