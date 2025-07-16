#include "mainwindow.h"
#include "models/mediaitem.h"
#include "models/upcomingitem.h"
#include "qjsonarray.h"
#include "storage/datamanager.h"
#include "ui_mainwindow.h"

#include <QStandardItemModel>
#include <QInputDialog>
#include <QModelIndex>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QDate>
#include <algorithm>
#include <QDialog>
#include <QVBoxLayout>
#include <QCalendarWidget>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Что я хочу посмотреть");
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dir.exists())
        dir.mkpath(".");

    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data");
    if (!dataDir.exists())
        dataDir.mkpath(".");
    // Инициализация моделей
    releasedModels["Сериалы"] = new QStandardItemModel(this);
    upcomingModel = new QStandardItemModel(this);
    trashModel = new QStandardItemModel(this);
    watchedModel = new QStandardItemModel(this);

    // Привязка моделей
    ui->listUpcoming->setModel(upcomingModel);
    ui->listTrash->setModel(trashModel);
    ui->listWatched->setModel(watchedModel);

    // Стартовые категории
    QStringList defaultCategories = {"Аниме", "Сериалы", "Фильмы"};
    for (const QString& cat : defaultCategories) {
        releasedModels[cat] = new QStandardItemModel(this);
        ui->comboCategoryReleased->addItem(cat);
    }


    // Добавляем категорию "Все" в начало списка и выбираем её по умолчанию
    ui->comboCategoryReleased->insertItem(0, "Все");
    currentReleasedCategory = "Все";
    ui->comboCategoryReleased->setCurrentText(currentReleasedCategory);

    // Поскольку "Все" — виртуальная категория, сразу показываем пустую модель
    ui->listReleased->setModel(new QStandardItemModel(this));

    // Переключение категорий
    connect(ui->comboCategoryReleased, &QComboBox::currentTextChanged, this, [=](const QString &text) {
        currentReleasedCategory = text;
        if (text == "Все") {
            updateAllReleasedModel();
        } else {
            ui->listReleased->setModel(releasedModels[text]);
        }
    });

    // Добавление новой категории
    connect(ui->btnAddCategory, &QPushButton::clicked, this, [=]() {
        bool ok;
        QString name = QInputDialog::getText(this, "Новая категория", "Название:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty() && !releasedModels.contains(name) && name != "Все") {
            releasedModels[name] = new QStandardItemModel(this);
            ui->comboCategoryReleased->addItem(name);
            saveData();

            if (currentReleasedCategory == "Все") {
                updateAllReleasedModel();
            }
        }
    });

    ui->listUpcoming->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->listReleased->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->listTrash->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    ui->listUpcoming->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->listTrash->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->listReleased->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Подключение кнопок "вышедшее"
    connect(ui->btnAddReleased, &QPushButton::clicked, this, &MainWindow::onAddReleased);
    connect(ui->btnEditReleased, &QPushButton::clicked, this, &MainWindow::onEditReleased);
    connect(ui->btnRemoveReleased, &QPushButton::clicked, this, &MainWindow::onRemoveReleased);
    connect(ui->listReleased, &QListView::doubleClicked, this, &MainWindow::onReleasedItemDoubleClicked);
    connect(ui->btnRemoveCategory, &QPushButton::clicked, this, &MainWindow::onRemoveCategoryClicked);

    // Подключение кнопок "не вышедшее"
    connect(ui->btnAddUpcoming, &QPushButton::clicked, this, &MainWindow::onAddUpcoming);
    connect(ui->btnEditUpcoming, &QPushButton::clicked, this, &MainWindow::onEditUpcoming);
    connect(ui->btnRemoveUpcoming, &QPushButton::clicked, this, &MainWindow::onRemoveUpcoming);

    // Подключение кнопок "шлак"
    connect(ui->btnAddTrash, &QPushButton::clicked, this, &MainWindow::onAddTrash);
    connect(ui->btnEditTrash, &QPushButton::clicked, this, &MainWindow::onEditTrash);
    connect(ui->btnRemoveTrash, &QPushButton::clicked, this, &MainWindow::onRemoveTrash);

     // Подключение кнопок "просмотренное"
    connect(ui->btnAddWatched, &QPushButton::clicked, this, &MainWindow::onAddWatched);
    connect(ui->btnEditWatched, &QPushButton::clicked, this, &MainWindow::onEditWatched);
    connect(ui->btnRemoveWatched, &QPushButton::clicked, this, &MainWindow::onRemoveWatched);

    setupContextMenus();

    setupThemeToggleButtons();
    loadData();                 // загрузка данных
    checkUpcomingToReleased();  // перенос по дате

}

MainWindow::~MainWindow()
{
    saveData();  // сохранение при выходе
    delete ui;
}

void MainWindow::onAddReleased()
{
    QString category = currentReleasedCategory;

    if (category == "Все") {
        QStringList categories = releasedModels.keys();
        categories.removeAll("Все");

        bool ok;
        category = QInputDialog::getItem(this, "Выбор категории", "Выберите категорию:", categories, 0, false, &ok);
        if (!ok || category.isEmpty()) return;
    }

    bool ok;
    QString text = QInputDialog::getText(this, "Добавить", "Введите название:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        releasedModels[category]->appendRow(new QStandardItem(text));
        saveData();
        if (currentReleasedCategory == "Все")
            updateAllReleasedModel();
    }
}

void MainWindow::onEditReleased()
{
    QModelIndex index = ui->listReleased->currentIndex();
    if (!index.isValid()) return;

    QString category = currentReleasedCategory;
    int row = index.row();

    if (category == "Все") {
        if (!proxyCategoryMap.contains(row)) return;
        category = proxyCategoryMap[row].first;
        row = proxyCategoryMap[row].second;
    }

    QStandardItemModel* model = releasedModels[category];
    if (!model || row >= model->rowCount()) return;

    QStandardItem* item = model->item(row);
    QString currentText = item->text();

    bool ok;
    QString newText = QInputDialog::getText(this, "Редактировать", "Изменить название:", QLineEdit::Normal, currentText.trimmed(), &ok);
    if (ok && !newText.isEmpty()) {
        item->setText(newText);
        saveData();
        if (currentReleasedCategory == "Все")
            updateAllReleasedModel();
    }
}


void MainWindow::onRemoveReleased()
{
    QModelIndex index = ui->listReleased->currentIndex();
    if (!index.isValid()) return;

    QString category = currentReleasedCategory;
    int row = index.row();

    if (category == "Все") {
        if (!proxyCategoryMap.contains(row)) return;
        category = proxyCategoryMap[row].first;
        row = proxyCategoryMap[row].second;
    }

    QStandardItemModel* model = releasedModels[category];
    if (!model || row >= model->rowCount()) return;

    model->removeRow(row);
    saveData();

    if (currentReleasedCategory == "Все")
        updateAllReleasedModel();
}


// Обновление модели для категории "Все"
void MainWindow::updateAllReleasedModel()
{
    QStandardItemModel *model = new QStandardItemModel(this);
    proxyCategoryMap.clear();  // очистим карту

    QStringList categories = releasedModels.keys();
    categories.removeAll("Все");
    std::sort(categories.begin(), categories.end());

    int row = 0;
    for (const QString &cat : categories) {
        QStandardItem *categoryItem = new QStandardItem(cat + ":");
        categoryItem->setFlags(Qt::NoItemFlags);
        model->appendRow(categoryItem);
        ++row;

        QStandardItemModel *catModel = releasedModels[cat];
        for (int i = 0; i < catModel->rowCount(); ++i) {
            QString indentedText = "    " + catModel->item(i)->text();
            QStandardItem *item = new QStandardItem(indentedText);
            model->appendRow(item);
            proxyCategoryMap[row] = {cat, i};  // сохраняем соответствие строк
            ++row;
        }
    }

    ui->listReleased->setModel(model);
}



// Двойной клик по элементу "Вышедшее"
void MainWindow::onReleasedItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    if (currentReleasedCategory == "Все") {
        // Найти к какой категории относится элемент
        QStringList categories = releasedModels.keys();
        categories.removeAll("Все");

        int row = index.row();
        int count = 0;

        for (const QString& cat : categories) {
            QStandardItemModel* model = releasedModels[cat];
            count++; // строка заголовка
            for (int i = 0; i < model->rowCount(); ++i) {
                if (count == row) {
                    QStandardItem* item = model->item(i);
                    if (!item) return;

                    QString text = item->text();
                    if (text.startsWith("🔔 ")) {
                        item->setText(text.mid(2));
                        saveData();
                        updateAllReleasedModel(); // обновим отображение
                    }
                    return;
                }
                count++;
            }
        }

        return; // если не нашли
    }

    // Обычное поведение
    QStandardItem *item = releasedModels[currentReleasedCategory]->itemFromIndex(index);
    if (!item) return;

    QString text = item->text();
    if (text.startsWith("🔔 ")) {
        text = text.mid(2);
        item->setText(text);
        saveData();  // не забываем сохранить
    }
}


// Удаление категории из "Вышедшее"
void MainWindow::onRemoveCategoryClicked()
{
    QString current = ui->comboCategoryReleased->currentText();

    if (current == "Все") {
        QMessageBox::warning(this, "Ошибка", "Категория 'Все' не может быть удалена.");
        return;
    }

    if (releasedModels.contains(current)) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Удалить категорию",
                                      "Вы уверены, что хотите удалить категорию '" + current + "'?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            delete releasedModels[current];
            releasedModels.remove(current);
            releasedItems.remove(current);

            int index = ui->comboCategoryReleased->currentIndex();
            ui->comboCategoryReleased->removeItem(index);

            // Обновляем текущую категорию
            if (ui->comboCategoryReleased->count() > 0) {
                QString newCat = ui->comboCategoryReleased->itemText(0);
                currentReleasedCategory = newCat;
                if (newCat == "Все") {
                    updateAllReleasedModel();
                } else {
                    ui->listReleased->setModel(releasedModels[newCat]);
                }
                ui->comboCategoryReleased->setCurrentText(newCat);
            } else {
                currentReleasedCategory.clear();
                ui->listReleased->setModel(nullptr);
            }

            saveData();
        }
    }
}

// Загрузка данных
void MainWindow::loadData()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath + "/data");

    // --- Загрузка вышедшего ---
    QString releasedFilePath = appDataPath + "/data/released.json";
    releasedItems.clear();
    for (auto model : releasedModels)
        model->clear();

    QFile fileReleased(releasedFilePath);
    if (fileReleased.open(QIODevice::ReadOnly)) {
        QByteArray data = fileReleased.readAll();
        fileReleased.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();

            // Удаляем категории, которых нет в файле (кроме "Все")
            QStringList existingCats = releasedModels.keys();
            for (const QString& cat : existingCats) {
                if (cat != "Все" && !rootObj.contains(cat)) {
                    delete releasedModels[cat];
                    releasedModels.remove(cat);
                    int idx = ui->comboCategoryReleased->findText(cat);
                    if (idx != -1)
                        ui->comboCategoryReleased->removeItem(idx);
                }
            }

            // Загружаем категории и элементы
            for (const QString& cat : rootObj.keys()) {
                if (cat == "Все") continue;

                if (!releasedModels.contains(cat)) {
                    releasedModels[cat] = new QStandardItemModel(this);
                    if (ui->comboCategoryReleased->findText(cat) == -1) {
                        ui->comboCategoryReleased->addItem(cat);
                    }
                }
                releasedModels[cat]->clear();

                if (rootObj[cat].isArray()) {
                    QJsonArray arr = rootObj[cat].toArray();
                    QVector<MediaItem> items;
                    for (const QJsonValue& val : arr) {
                        MediaItem item;
                        item.title = val.toString();
                        items.append(item);

                        QStandardItem* stdItem = new QStandardItem(item.title);
                        releasedModels[cat]->appendRow(stdItem);
                    }
                    releasedItems[cat] = items;
                } else {
                    releasedItems[cat].clear();
                }
            }
        }
    }

    updateAllReleasedModel();

    // --- Загрузка не вышедшего ---
    QString upcomingFilePath = appDataPath + "/data/upcoming.json";
    upcomingItems = DataManager::loadUpcomingItems(upcomingFilePath);
    updateUpcomingModel();

    // --- Загрузка шлака ---
    QString trashFilePath = appDataPath + "/data/trash.json";
    trashItems = DataManager::loadItems(trashFilePath);
    trashModel->clear();
    for (const MediaItem& item : trashItems) {
        QStandardItem* trash = new QStandardItem(item.title);
        trash->setFlags(trash->flags() & ~Qt::ItemIsEditable);
        trashModel->appendRow(trash);
    }

    // --- Загрузка просмотренного ---
    QString watchedFilePath = appDataPath + "/data/watched.json";
    watchedItems = DataManager::loadWatchedItems();
    watchedModel->clear();
    for (const QString& title : watchedItems) {
        QStandardItem* watched = new QStandardItem(title);
        watched->setFlags(watched->flags() & ~Qt::ItemIsEditable);
        watchedModel->appendRow(watched);
    }
}

void MainWindow::saveData()
{
    // Сохраняем просмотренное
    watchedItems.clear();
    for (int i = 0; i < watchedModel->rowCount(); ++i) {
        MediaItem item;
        item.title = watchedModel->item(i)->text();
        watchedItems.append(watchedModel->item(i)->text());

    }

    // Сохраняем вышедшее
    for (const QString& cat : releasedModels.keys()) {
        if (cat == "Все") continue;
        releasedItems[cat].clear();
        QStandardItemModel* model = releasedModels[cat];
        for (int i = 0; i < model->rowCount(); ++i) {
            MediaItem item;
            item.title = model->item(i)->text();
            releasedItems[cat].append(item);
        }
    }

    // Сохраняем released.json
    QJsonObject rootObj;
    for (const QString& cat : releasedItems.keys()) {
        if (cat == "Все") continue;
        QJsonArray arr;
        for (const MediaItem& item : releasedItems[cat]) {
            arr.append(item.title);
        }
        rootObj[cat] = arr;
    }
    QJsonDocument doc(rootObj);
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath + "/data");
    QFile fileReleased(appDataPath + "/data/released.json");
    if (fileReleased.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fileReleased.write(doc.toJson());
        fileReleased.close();
    }

    // Сохраняем не вышедшее
    QString upcomingFilePath = appDataPath + "/data/upcoming.json";
    DataManager::saveUpcomingItems(upcomingFilePath, upcomingItems);

    // Сохраняем шлак
    trashItems.clear();
    for (int i = 0; i < trashModel->rowCount(); ++i) {
        MediaItem item;
        item.title = trashModel->item(i)->text();
        trashItems.append(item);
    }
    QString trashFilePath = appDataPath + "/data/trash.json";
    DataManager::saveItems(trashFilePath, trashItems);

    // Сохраняем просмотренное
    QString watchedFilePath = appDataPath + "/data/watched.json";
    DataManager::saveWatchedItems(watchedItems);
}

void MainWindow::updateUpcomingModel()
{
    upcomingModel->clear();
    std::sort(upcomingItems.begin(), upcomingItems.end());

    for (const UpcomingItem& item : upcomingItems) {
        QStandardItem* stdItem = new QStandardItem(item.displayText());
        stdItem->setFlags(stdItem->flags() & ~Qt::ItemIsEditable);
        upcomingModel->appendRow(stdItem);
    }
}

void MainWindow::onAddUpcoming()
{
    UpcomingItem item;

    bool ok;
    item.title = QInputDialog::getText(this, "Добавить", "Название:", QLineEdit::Normal, "", &ok);
    if (!ok || item.title.isEmpty()) return;

    QString seasonStr = QInputDialog::getText(this, "Добавить", "Сезон (опц.):", QLineEdit::Normal, "", &ok);
    if (ok) {
        item.season = seasonStr.trimmed().isEmpty() ? -1 : seasonStr.toInt();
    }

    QString episodeStr = QInputDialog::getText(this, "Добавить", "Серия (опц.):", QLineEdit::Normal, "", &ok);
    if (ok) {
        item.episode = episodeStr.trimmed().isEmpty() ? -1 : episodeStr.toInt();
    }

    QDate selectedDate;
    bool unknownDate = false;
    if (!getDateWithOptionalUnknown(selectedDate, unknownDate))
        return;

    item.date = selectedDate;
    item.dateUnknown = unknownDate;

    // Выбор категории
    QStringList categories = releasedModels.keys();
    categories.removeAll("Все");
    std::sort(categories.begin(), categories.end());
    item.category = QInputDialog::getItem(this, "Категория", "Выберите категорию:", categories, 0, false, &ok);
    if (!ok || item.category.isEmpty()) return;

    upcomingItems.append(item);
    updateUpcomingModel();
}


void MainWindow::onEditUpcoming()
{
    QModelIndex index = ui->listUpcoming->currentIndex();
    if (!index.isValid()) return;

    int row = index.row();
    if (row < 0 || row >= upcomingItems.size()) return;

    UpcomingItem &item = upcomingItems[row];

    bool ok;
    QString text = QInputDialog::getText(this, "Редактировать", "Название:", QLineEdit::Normal, item.title, &ok);
    if (!ok || text.isEmpty()) return;
    item.title = text;

    QString seasonStr = QInputDialog::getText(this, "Редактировать", "Сезон (опц.):", QLineEdit::Normal,
                                              item.season >= 0 ? QString::number(item.season) : "", &ok);
    if (ok) {
        item.season = seasonStr.isEmpty() ? -1 : seasonStr.toInt();
    }

    QString episodeStr = QInputDialog::getText(this, "Редактировать", "Серия (опц.):", QLineEdit::Normal,
                                               item.episode >= 0 ? QString::number(item.episode) : "", &ok);
    if (ok) {
        item.episode = episodeStr.isEmpty() ? -1 : episodeStr.toInt();
    }

    QDate selectedDate = item.dateUnknown ? QDate::currentDate() : item.date;
    bool unknownDate = item.dateUnknown;

    if (!getDateWithOptionalUnknown(selectedDate, unknownDate))
        return;

    item.date = selectedDate;
    item.dateUnknown = unknownDate;


    updateUpcomingModel();
}

void MainWindow::onRemoveUpcoming()
{
    QModelIndex index = ui->listUpcoming->currentIndex();
    if (!index.isValid()) return;

    int row = index.row();
    if (row < 0 || row >= upcomingItems.size()) return;

    upcomingItems.removeAt(row);
    updateUpcomingModel();
}


void MainWindow::onAddTrash()
{
    bool ok;
    QString text = QInputDialog::getText(this, "Добавить", "Введите название:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        trashModel->appendRow(new QStandardItem(text));
    }
}

void MainWindow::onEditTrash()
{
    QModelIndex index = ui->listTrash->currentIndex();
    if (!index.isValid()) return;

    QString currentText = trashModel->itemFromIndex(index)->text();

    bool ok;
    QString text = QInputDialog::getText(this, "Редактировать", "Изменить название:", QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        trashModel->itemFromIndex(index)->setText(text);
    }
}

void MainWindow::onRemoveTrash()
{
    QModelIndex index = ui->listTrash->currentIndex();
    if (index.isValid()) {
        trashModel->removeRow(index.row());
    }
}

void MainWindow::onAddWatched()
{
    bool ok;
    QString text = QInputDialog::getText(this, "Добавить", "Введите название:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        watchedModel->appendRow(new QStandardItem(text));
        saveData();
    }
}

void MainWindow::onEditWatched()
{
    QModelIndex index = ui->listWatched->currentIndex();
    if (!index.isValid()) return;

    QString currentText = watchedModel->itemFromIndex(index)->text();
    bool ok;
    QString newText = QInputDialog::getText(this, "Редактировать", "Изменить название:", QLineEdit::Normal, currentText, &ok);
    if (ok && !newText.isEmpty()) {
        watchedModel->itemFromIndex(index)->setText(newText);
        saveData();
    }
}

void MainWindow::onRemoveWatched()
{
    QModelIndex index = ui->listWatched->currentIndex();
    if (index.isValid()) {
        watchedModel->removeRow(index.row());
        saveData();
    }
}

void MainWindow::checkUpcomingToReleased()
{
    QDate today = QDate::currentDate();
    QVector<int> toRemove;

    for (int i = 0; i < upcomingItems.size(); ++i) {
        UpcomingItem &item = upcomingItems[i];

        if (!item.dateUnknown && item.date <= today && !item.transferred) {
            QString category = item.category;
            if (!releasedModels.contains(category)) {
                releasedModels[category] = new QStandardItemModel(this);
                ui->comboCategoryReleased->addItem(category);
            }

            // Добавление в визуальный список
            QString text = item.title;
            if (item.season >= 0) {
                text += " | Сезон " + QString::number(item.season);
            }
            text += " | ";
            if (item.episode >= 0) {
                text += " Серия " + QString::number(item.episode) + " | ";
            }

            QStandardItem *newItem = new QStandardItem("🔔 " + text);
            releasedModels[category]->appendRow(newItem);

            // Сохранение полного MediaItem в releasedItems
            MediaItem mediaItem;
            mediaItem.title = item.title;
            mediaItem.season = item.season;
            mediaItem.episode = item.episode;
            mediaItem.releaseDate = item.date;
            mediaItem.dateUnknown = item.dateUnknown;
            mediaItem.category = Category::Released;

            // Устанавливаем тип по строке категории
            if (item.category == "Аниме")
                mediaItem.type = MediaType::Anime;
            else if (item.category == "Фильмы")
                mediaItem.type = MediaType::Movie;
            else
                mediaItem.type = MediaType::Series;

            releasedItems[category].append(mediaItem);

            // Отметить как перенесённый
            item.transferred = true;
            toRemove.append(i);
        }
    }

    // Удалить перенесённые из списка "не вышедших"
    for (int i = toRemove.size() - 1; i >= 0; --i) {
        upcomingItems.removeAt(toRemove[i]);
    }

    updateUpcomingModel();
    saveData();
    if (currentReleasedCategory == "Все") {
        updateAllReleasedModel();
    }
}

void MainWindow::applyDarkTheme() {
    qApp->setStyleSheet(R"(
    QWidget {
        background-color: #2b2b2b;
        color: #f0f0f0;
    }

    QTabWidget::pane {
        border: 1px solid #444;
        background: #2b2b2b;
    }

    QTabBar::tab {
        background: #3c3f41;
        color: #f0f0f0;
        padding: 5px;
        border: 1px solid #555;
        border-bottom: none;
        border-top-left-radius: 4px;
        border-top-right-radius: 4px;
    }

    QTabBar::tab:selected {
        background: #555;
    }

    QScrollBar:vertical {
        background: #2b2b2b;
    }

    QListView::item:selected,
    QTreeView::item:selected,
    QTableView::item:selected,
    QCalendarWidget QAbstractItemView::item:selected {
        background-color: #3399ff;
        color: #ffffff;
    }

    QComboBox QAbstractItemView::item:selected {
        background-color: #3399ff;
        color: white;
    }

    QCalendarWidget QWidget#qt_calendar_navigationbar {
        background-color: #3c3f41;
    }

    /* Контекстное меню */
    QMenu {
        background-color: #2b2b2b;
        border: 1px solid #444;
        color: #f0f0f0;
    }

    QMenu::item {
        padding: 4px 24px 4px 24px;
        background-color: transparent;
    }

    QMenu::item:selected {
        background-color: #3399ff; /* светло-голубая подсветка */
        color: white;
    }
)");
}


void MainWindow::applyLightTheme() {
    qApp->setStyleSheet("");
}

void MainWindow::setupThemeToggleButtons()
{
    QList<QPushButton*> buttons = {
        ui->btnToggleTheme,
        ui->btnToggleTheme_2,
        ui->btnToggleTheme_3
    };

    // Стилизация и включение режима checkable
    for (QPushButton* btn : buttons) {
        if (!btn) continue;

        btn->setCheckable(true);
        btn->setStyleSheet(R"(
            QPushButton {
                border: 1px solid #999;
                border-radius: 14px;
                padding: 4px 10px;
                background-color: #ccc;
                color: black;
                font-size: 16px;
            }
            QPushButton:checked {
                background-color: #444;
                color: white;
            }
        )");
    }

    // Загрузка состояния темы
    QSettings settings("sendy-tech", "SerialNotes");
    isDarkTheme = settings.value("darkTheme", false).toBool();

    if (isDarkTheme)
        applyDarkTheme();
    else
        applyLightTheme();

    // Установка начального состояния кнопок
    for (QPushButton* btn : buttons) {
        if (!btn) continue;
        btn->blockSignals(true);
        btn->setChecked(isDarkTheme);
        btn->setText(isDarkTheme ? "🌞" : "🌙");
        btn->blockSignals(false);
    }

    // Подключение всех кнопок к одному слоту
    for (QPushButton* btn : buttons) {
        if (!btn) continue;

        connect(btn, &QPushButton::toggled, this, [=](bool checked) {
            isDarkTheme = checked;

            // Применить ко всем кнопкам
            for (QPushButton* b : buttons) {
                if (!b) continue;
                b->blockSignals(true);
                b->setChecked(checked);
                b->setText(checked ? "🌞" : "🌙");
                b->blockSignals(false);
            }

            if (checked)
                applyDarkTheme();
            else
                applyLightTheme();

            QSettings settings("sendy-tech", "SerialNotes");
            settings.setValue("darkTheme", isDarkTheme);
        });
    }
}

bool MainWindow::getDateWithOptionalUnknown(QDate& selectedDate, bool& isUnknown) {
    QDialog dialog(this);
    dialog.setWindowTitle("Выберите дату");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QCalendarWidget* calendar = new QCalendarWidget(&dialog);
    calendar->setGridVisible(true);
    layout->addWidget(calendar);

    QCheckBox* unknownCheckBox = new QCheckBox("Дата неизвестна", &dialog);
    layout->addWidget(unknownCheckBox);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(unknownCheckBox, &QCheckBox::toggled, calendar, &QCalendarWidget::setDisabled);

    if (dialog.exec() == QDialog::Accepted) {
        isUnknown = unknownCheckBox->isChecked();
        if (!isUnknown)
            selectedDate = calendar->selectedDate();
        return true;
    }

    return false;
}

void MainWindow::setupContextMenus()
{
    ui->listReleased->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listReleased, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        QModelIndex index = ui->listReleased->indexAt(pos);
        if (!index.isValid()) return;

        QMenu menu(this);
        menu.addAction("Редактировать", this, &MainWindow::onEditReleasedContext);
        menu.addAction("Удалить", this, &MainWindow::onRemoveReleasedContext);
        menu.addAction("В шлак", this, &MainWindow::onMoveReleasedToTrash);
        menu.addAction("В просмотренное", this, &MainWindow::onMoveReleasedToWatched);
        menu.exec(ui->listReleased->viewport()->mapToGlobal(pos));
    });

    ui->listUpcoming->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listUpcoming, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        QModelIndex index = ui->listUpcoming->indexAt(pos);
        if (!index.isValid()) return;

        QMenu menu(this);
        menu.addAction("Редактировать", this, &MainWindow::onEditUpcomingContext);
        menu.addAction("Удалить", this, &MainWindow::onRemoveUpcomingContext);
        menu.exec(ui->listUpcoming->viewport()->mapToGlobal(pos));
    });

    ui->listTrash->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listTrash, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        QModelIndex index = ui->listTrash->indexAt(pos);
        if (!index.isValid()) return;

        QMenu menu(this);
        menu.addAction("Редактировать", this, &MainWindow::onEditTrashContext);
        menu.addAction("Удалить", this, &MainWindow::onRemoveTrashContext);
        menu.exec(ui->listTrash->viewport()->mapToGlobal(pos));
    });

    ui->listWatched->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWatched, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        QModelIndex index = ui->listWatched->indexAt(pos);
        if (!index.isValid()) return;

        QMenu menu(this);
        menu.addAction("Редактировать", this, &MainWindow::onEditWatched);
        menu.addAction("Удалить", this, &MainWindow::onRemoveWatched);
        menu.exec(ui->listWatched->viewport()->mapToGlobal(pos));
    });
}

// --- Обработчики для "Вышедшее"
void MainWindow::onEditReleasedContext()
{
    // Просто вызываем уже существующий метод редактирования
    onEditReleased();
}

void MainWindow::onRemoveReleasedContext()
{
    onRemoveReleased();
}

void MainWindow::onMoveReleasedToTrash()
{
    QModelIndex index = ui->listReleased->currentIndex();
    if (!index.isValid()) return;

    QString category = currentReleasedCategory;
    int row = index.row();

    if (category == "Все") {
        if (!proxyCategoryMap.contains(row)) return;
        category = proxyCategoryMap[row].first;
        row = proxyCategoryMap[row].second;
    }

    QStandardItemModel* model = releasedModels[category];
    if (!model || row >= model->rowCount()) return;

    QStandardItem* item = model->item(row);
    if (!item) return;

    QString text = item->text();

    // Добавляем в шлак
    trashModel->appendRow(new QStandardItem(text));

    // Удаляем из released
    model->removeRow(row);

    saveData();

    if (currentReleasedCategory == "Все")
        updateAllReleasedModel();
}

void MainWindow::onMoveReleasedToWatched()
{
    QModelIndex index = ui->listReleased->currentIndex();
    if (!index.isValid()) return;

    QString category = currentReleasedCategory;
    int row = index.row();

    if (category == "Все") {
        if (!proxyCategoryMap.contains(row)) return;
        category = proxyCategoryMap[row].first;
        row = proxyCategoryMap[row].second;
    }

    QStandardItemModel* model = releasedModels[category];
    if (!model || row >= model->rowCount()) return;

    QStandardItem* item = model->item(row);
    if (!item) return;

    QString text = item->text();

    // Добавляем в просмотренное
    watchedModel->appendRow(new QStandardItem(text));

    // Удаляем из released
    model->removeRow(row);

    saveData();

    if (currentReleasedCategory == "Все")
        updateAllReleasedModel();
}


// --- Обработчики для "Не вышедшее"
void MainWindow::onEditUpcomingContext()
{
    onEditUpcoming();
}

void MainWindow::onRemoveUpcomingContext()
{
    onRemoveUpcoming();
}

// --- Обработчики для "Шлак"
void MainWindow::onEditTrashContext()
{
    onEditTrash();
}

void MainWindow::onRemoveTrashContext()
{
    onRemoveTrash();
}

void MainWindow::updateWatchedModel(const QStringList &watchedItems)
{
    watchedModel->clear();
    for (const QString &title : watchedItems) {
        watchedModel->appendRow(new QStandardItem(title));
    }
}
