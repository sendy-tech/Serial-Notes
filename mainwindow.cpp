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

    // Привязка моделей
    ui->listUpcoming->setModel(upcomingModel);
    ui->listTrash->setModel(trashModel);

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
        }
    });

    ui->listUpcoming->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->listReleased->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->listTrash->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

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

    setupThemeToggleButtons();
    loadData();                 // загрузка данных
    checkUpcomingToReleased();  // перенос по дате

}

MainWindow::~MainWindow()
{
    saveData();  // сохранение при выходе
    delete ui;
}

// Слот: Добавить элемент во "Вышедшее"
void MainWindow::onAddReleased()
{
    if (currentReleasedCategory == "Все") {
        QMessageBox::warning(this, "Ошибка", "Нельзя добавлять элемент напрямую в категорию 'Все'. Выберите конкретную категорию.");
        return;
    }

    bool ok;
    QString text = QInputDialog::getText(this, "Добавить", "Введите название:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        releasedModels[currentReleasedCategory]->appendRow(new QStandardItem(text));
        saveData(); // сохранить изменения
    }
}

void MainWindow::onEditReleased()
{
    if (currentReleasedCategory == "Все") {
        QMessageBox::warning(this, "Ошибка", "Редактирование элементов из категории 'Все' не поддерживается. Выберите конкретную категорию.");
        return;
    }

    QModelIndex index = ui->listReleased->currentIndex();
    if (!index.isValid()) return;

    QString currentText = releasedModels[currentReleasedCategory]->itemFromIndex(index)->text();

    bool ok;
    QString text = QInputDialog::getText(this, "Редактировать", "Изменить название:", QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        releasedModels[currentReleasedCategory]->itemFromIndex(index)->setText(text);
        saveData();  // Сохраняем изменения сразу после редактирования
    }
}

void MainWindow::onRemoveReleased()
{
    if (currentReleasedCategory == "Все") {
        QMessageBox::warning(this, "Ошибка", "Удаление элементов из категории 'Все' не поддерживается. Выберите конкретную категорию.");
        return;
    }

    QModelIndex index = ui->listReleased->currentIndex();
    if (index.isValid()) {
        releasedModels[currentReleasedCategory]->removeRow(index.row());
        saveData();  // Сохраняем изменения сразу после удаления
    }
}

// Обновление модели для категории "Все"
void MainWindow::updateAllReleasedModel()
{
    auto *model = new QStandardItemModel(this);

    QStringList categories = releasedModels.keys();
    categories.removeAll("Все"); // исключаем "Все" из списка категорий
    std::sort(categories.begin(), categories.end());

    for (const QString &cat : categories) {
        QStandardItem *categoryItem = new QStandardItem(cat + ":");
        categoryItem->setFlags(Qt::NoItemFlags);  // Заголовок некликабельный
        model->appendRow(categoryItem);

        QStandardItemModel *catModel = releasedModels[cat];
        for (int i = 0; i < catModel->rowCount(); ++i) {
            QString indentedText = "    " + catModel->item(i)->text(); // табуляция 4 пробела
            QStandardItem *item = new QStandardItem(indentedText);
            model->appendRow(item);
        }
    }

    ui->listReleased->setModel(model);
}


// Двойной клик по элементу "Вышедшее"
void MainWindow::onReleasedItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    if (currentReleasedCategory == "Все") {
        // Для простоты ничего не делаем при двойном клике в категории "Все"
        return;
    }

    QStandardItem *item = releasedModels[currentReleasedCategory]->itemFromIndex(index);
    if (!item) return;

    QString text = item->text();
    if (text.startsWith("🔔 ")) {
        text = text.mid(2);
        item->setText(text);
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
        }
    }
}

// Загрузка данных
void MainWindow::loadData()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists())
        dir.mkpath(".");

    QDir dataDir(appDataPath + "/data");
    if (!dataDir.exists())
        dataDir.mkpath(".");

    QString releasedFilePath = dataDir.filePath("released.json");

    releasedItems.clear();
    for (auto model : releasedModels)
        model->clear();

    QFile file(releasedFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();

            // Удаляем все категории, кроме "Все" (виртуальной)
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

                        releasedModels[cat]->appendRow(new QStandardItem(item.title));
                    }
                    releasedItems[cat] = items;
                } else {
                    releasedItems[cat].clear();
                }
            }
        }
    }

    updateAllReleasedModel();

    // Загрузка не вышедшего (upcoming)
    QString upcomingFilePath = dataDir.filePath("upcoming.json");
    upcomingItems = DataManager::loadUpcomingItems(upcomingFilePath);
    updateUpcomingModel();

    // Загрузка шлака (trash)
    QString trashFilePath = dataDir.filePath("trash.json");
    trashItems = DataManager::loadItems(trashFilePath);
    trashModel->clear();
    for (const MediaItem& item : trashItems) {
        trashModel->appendRow(new QStandardItem(item.title));
    }
}



// Сохранение данных
void MainWindow::saveData()
{
    // Обновляем releasedItems из моделей
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

    // Сохраняем releasedItems в released.json
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
    QString releasedFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data/released.json";
    QFile file(releasedFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson());
        file.close();
    }

    // Сохранение не вышедшего (upcoming)
    QString upcomingFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data/upcoming.json";
    DataManager::saveUpcomingItems(upcomingFilePath, upcomingItems);

    // Сохранение шлака (trash)
    trashItems.clear();
    for (int i = 0; i < trashModel->rowCount(); ++i) {
        MediaItem item;
        item.title = trashModel->item(i)->text();
        trashItems.append(item);
    }
    QString trashFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data/trash.json";
    DataManager::saveItems(trashFilePath, trashItems);
}

void MainWindow::updateUpcomingModel()
{
    upcomingModel->clear();
    std::sort(upcomingItems.begin(), upcomingItems.end());

    for (const UpcomingItem& item : upcomingItems)
        upcomingModel->appendRow(new QStandardItem(item.displayText()));
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
    QStringList categories = {"Аниме", "Сериалы", "Фильмы"};
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
                text += " Сезон" + QString::number(item.season);
            }
            if (item.episode >= 0) {
                text += " Серия" + QString::number(item.episode);
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
