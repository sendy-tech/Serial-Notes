#ifndef UPCOMINGITEM_H
#define UPCOMINGITEM_H

#include <QString>
#include <QDate>

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
};

#endif
