#ifndef UPCOMINGITEM_H
#define UPCOMINGITEM_H

#include <QString>
#include <QDate>
#include <QJsonObject>

struct UpcomingItem {
    QString title;
    int season = -1;
    int episode = -1;
    QDate date;
    bool dateUnknown = true;
    bool transferred = false;
    QString category;  // "Аниме", "Сериал", "Фильм"

    QString displayText() const {
        QString result = title;
        if (season >= 0)
            result += QString(" | Сезон %1").arg(season);
        if (episode >= 0)
            result += QString(", Серия %1").arg(episode);
        if (!dateUnknown)
            result += QString(" | %1").arg(date.toString("dd.MM.yyyy"));
        else
            result += " | Дата неизвестна";

        if (!category.isEmpty())
            result += QString(" [%1]").arg(category);

        return result;
    }

    bool operator<(const UpcomingItem& other) const {
        if (dateUnknown && !other.dateUnknown) return false;
        if (!dateUnknown && other.dateUnknown) return true;
        return date < other.date;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["title"] = title;
        obj["season"] = season;
        obj["episode"] = episode;
        obj["date"] = date.toString(Qt::ISODate); // безопасный формат
        obj["dateUnknown"] = dateUnknown;
        obj["transferred"] = transferred;
        obj["category"] = category;
        return obj;
    }

    static UpcomingItem fromJson(const QJsonObject &obj) {
        UpcomingItem item;
        item.title = obj["title"].toString();
        item.season = obj["season"].toInt(-1);
        item.episode = obj["episode"].toInt(-1);
        item.date = QDate::fromString(obj["date"].toString(), Qt::ISODate);
        item.dateUnknown = obj["dateUnknown"].toBool(true);
        item.transferred = obj["transferred"].toBool(false);
        item.category = obj["category"].toString();
        return item;
    }
};

#endif
