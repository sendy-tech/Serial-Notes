#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QMap>
#include <QVector>
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

    // Хранилище моделей по категориям
    QMap<QString, QStandardItemModel*> releasedModels;
    QString currentReleasedCategory = "Сериалы";

    // Модель для "Не вышедшее"
    QStandardItemModel *upcomingModel;

    // Модель для "Шлак"
    QStandardItemModel *trashModel;

    // Данные для "Не вышедшее"
    QVector<UpcomingItem> upcomingItems;

    // Перестраивает модель upcomingModel из вектора
    void updateUpcomingModel();

    // Проверка на перенос из Upcoming → Released
    void checkUpcomingToReleased();

    void loadData();
    void saveData();

    // Добавим поля для хранения данных
    QMap<QString, QVector<MediaItem>> releasedItems; // категория -> список
    QVector<MediaItem> trashItems;

private slots:
    // Вышедшее
    void onAddReleased();
    void onEditReleased();
    void onRemoveReleased();
    void onReleasedItemDoubleClicked(const QModelIndex &index);
    void onRemoveCategoryClicked();

    // Не вышедшее
    void onAddUpcoming();
    void onEditUpcoming();
    void onRemoveUpcoming();

    // Шлак
    void onAddTrash();
    void onEditTrash();
    void onRemoveTrash();
};

#endif // MAINWINDOW_H
