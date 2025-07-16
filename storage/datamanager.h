#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "models/MediaItem.h"
#include "models/UpcomingItem.h"
#include <QVector>
#include <QString>

class DataManager
{
public:
    // Для MediaItem
    static QVector<MediaItem> loadItems(const QString &filePath);
    static void saveItems(const QString &filePath, const QVector<MediaItem> &items);

    // Для UpcomingItem
    static QVector<UpcomingItem> loadUpcomingItems(const QString &filePath);
    static void saveUpcomingItems(const QString &filePath, const QVector<UpcomingItem> &items);


    // Для wathed
    static QStringList loadWatchedItems();
    static void saveWatchedItems(const QStringList &items);
};

#endif
