#ifndef MEDIAITEM_H
#define MEDIAITEM_H

#include <QString>
#include <QDate>
#include <QJsonObject>

enum class MediaType { Series, Anime, Movie };
enum class Category { Released, Upcoming, Trash };

class MediaItem {
public:
    QString title;
    MediaType type;
    Category category;
    int season;
    int episode;
    QDate releaseDate;
    bool dateUnknown;

    MediaItem();
    MediaItem(const QJsonObject& json);

    QJsonObject toJson() const;

    // Добавляем статический метод fromJson
    static MediaItem fromJson(const QJsonObject& json);
};

#endif
