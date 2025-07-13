#include "mediaitem.h"

MediaItem::MediaItem()
{
    season = -1;
    episode = -1;
    dateUnknown = true;
}

MediaItem::MediaItem(const QJsonObject &json)
{
    title = json["title"].toString();
    type = static_cast<MediaType>(json["type"].toInt());
    category = static_cast<Category>(json["category"].toInt());
    season = json["season"].toInt(-1);
    episode = json["episode"].toInt(-1);

    if (json.contains("releaseDate"))
        releaseDate = QDate::fromString(json["releaseDate"].toString(), Qt::ISODate);

    dateUnknown = json["dateUnknown"].toBool(true);
}

QJsonObject MediaItem::toJson() const
{
    QJsonObject obj;
    obj["title"] = title;
    obj["type"] = static_cast<int>(type);
    obj["category"] = static_cast<int>(category);
    obj["season"] = season;
    obj["episode"] = episode;
    obj["releaseDate"] = releaseDate.toString(Qt::ISODate);
    obj["dateUnknown"] = dateUnknown;
    return obj;
}
