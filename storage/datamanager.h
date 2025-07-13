#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "models/MediaItem.h"
#include <QVector>

class DataManager
{
public:
    static QVector<MediaItem> loadItems(const QString &filePath);
    static void saveItems(const QString &filePath, const QVector<MediaItem> &items);
};

#endif
