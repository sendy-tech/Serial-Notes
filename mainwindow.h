#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QMap>
#include <QVector>
#include <QSettings>
#include <QPair>
#include "models/upcomingitem.h"
#include "models/mediaitem.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QMap<QString, QStandardItemModel*> releasedModels;
    QString currentReleasedCategory = "Сериалы";

    QStandardItemModel *upcomingModel;
    QStandardItemModel *trashModel;
    QStandardItemModel* watchedModel;

    QStringList watchedItems;

    QVector<UpcomingItem> upcomingItems;
    QMap<QString, QVector<MediaItem>> releasedItems;
    QVector<MediaItem> trashItems;

    bool isDarkTheme = false;

    QMap<int, QPair<QString, int>> proxyCategoryMap;

    void updateUpcomingModel();
    void checkUpcomingToReleased();
    void loadData();
    void saveData();
    void applyDarkTheme();
    void applyLightTheme();
    void setupThemeToggleButtons();
    bool getDateWithOptionalUnknown(QDate& selectedDate, bool& isUnknown);
    void setupContextMenus();

    void onEditReleasedContext();
    void onRemoveReleasedContext();
    void onMoveReleasedToTrash();
    void onMoveReleasedToWatched();

    void onEditUpcomingContext();
    void onRemoveUpcomingContext();

    void onEditTrashContext();
    void onRemoveTrashContext();

    void updateWatchedModel(const QStringList &watchedItems);


private slots:
    // Вышедшее
    void onAddReleased();
    void onEditReleased();
    void onRemoveReleased();
    void onReleasedItemDoubleClicked(const QModelIndex &index);
    void onRemoveCategoryClicked();
    void updateAllReleasedModel();

    // Не вышедшее
    void onAddUpcoming();
    void onEditUpcoming();
    void onRemoveUpcoming();

    // Шлак
    void onAddTrash();
    void onEditTrash();
    void onRemoveTrash();

    // Просмотренное
    void onAddWatched();
    void onEditWatched();
    void onRemoveWatched();


};

#endif // MAINWINDOW_H
