#include "mainwindow.h"
#include "models/mediaitem.h"
#include "models/upcomingitem.h"
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

// Пути к файлам
QString releasedPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data/released.json";
QString upcomingPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data/upcoming.json";
QString trashPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data/trash.json";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Что я хочу посмотреть");

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

    ui->comboCategoryReleased->setCurrentText(currentReleasedCategory);
    ui->listReleased->setModel(releasedModels[currentReleasedCategory]);

    // Переключение категорий
    connect(ui->comboCategoryReleased, &QComboBox::currentTextChanged, this, [=](const QString &text) {
        currentReleasedCategory = text;
        ui->listReleased->setModel(releasedModels[text]);
    });

    // Добавление новой категории
    connect(ui->btnAddCategory, &QPushButton::clicked, this, [=]() {
        bool ok;
        QString name = QInputDialog::getText(this, "Новая категория", "Название:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty() && !releasedModels.contains(name)) {
            releasedModels[name] = new QStandardItemModel(this);
            ui->comboCategoryReleased->addItem(name);
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
    checkUpcomingToReleased();  // перенос по дате
    loadData();                 // загрузка данных
}

MainWindow::~MainWindow()
{
    saveData();  // сохранение при выходе
    delete ui;
}

// Слот: Добавить элемент
void MainWindow::onAddReleased()
{
    bool ok;
    QString text = QInputDialog::getText(this, "Добавить", "Введите название:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        releasedModels[currentReleasedCategory]->appendRow(new QStandardItem(text));
    }
}

// Слот: Редактировать выбранный элемент
void MainWindow::onEditReleased()
{
    QModelIndex index = ui->listReleased->currentIndex();
    if (!index.isValid()) return;

    QString currentText = releasedModels[currentReleasedCategory]->itemFromIndex(index)->text();

    bool ok;
    QString text = QInputDialog::getText(this, "Редактировать", "Изменить название:", QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        releasedModels[currentReleasedCategory]->itemFromIndex(index)->setText(text);
    }
}

// Слот: Удалить выбранный элемент
void MainWindow::onRemoveReleased()
{
    QModelIndex index = ui->listReleased->currentIndex();
    if (index.isValid()) {
        releasedModels[currentReleasedCategory]->removeRow(index.row());
    }
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
    item.season = ok ? seasonStr.toInt() : -1;

    QString episodeStr = QInputDialog::getText(this, "Добавить", "Серия (опц.):", QLineEdit::Normal, "", &ok);
    item.episode = ok ? episodeStr.toInt() : -1;

    QString dateStr = QInputDialog::getText(this, "Добавить", "Дата выхода (ГГГГ-ММ-ДД, можно частично):", QLineEdit::Normal, "", &ok);
    if (ok && !dateStr.isEmpty()) {
        QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
        if (date.isValid()) {
            item.date = date;
            item.dateUnknown = false;
        }
    }

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

    QString dateStr = QInputDialog::getText(this, "Редактировать", "Дата выхода (ГГГГ-ММ-ДД, можно частично):", QLineEdit::Normal,
                                            item.dateUnknown ? "" : item.date.toString("yyyy-MM-dd"), &ok);
    if (ok && !dateStr.isEmpty()) {
        QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
        if (date.isValid()) {
            item.date = date;
            item.dateUnknown = false;
        } else {
            item.dateUnknown = true;
        }
    } else if (ok && dateStr.isEmpty()) {
        item.dateUnknown = true;
    }

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
            QStandardItem *newItem = new QStandardItem("🔔 " + item.displayText());
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
}

void MainWindow::onReleasedItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QStandardItem *item = releasedModels[currentReleasedCategory]->itemFromIndex(index);
    if (!item) return;

    QString text = item->text();
    if (text.startsWith("🔔 ")) {
        text = text.mid(2);  // Удалить значок и пробел
        item->setText(text);
    }
}

void MainWindow::onRemoveCategoryClicked()
{
    QString current = ui->comboCategoryReleased->currentText();
    if (releasedModels.contains(current)) {
        // Подтверждение
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Удалить категорию",
                                      "Вы уверены, что хотите удалить категорию '" + current + "'?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            // Удалить модель и виджет
            delete releasedModels[current];
            releasedModels.remove(current);

            int index = ui->comboCategoryReleased->currentIndex();
            ui->comboCategoryReleased->removeItem(index);

            // Обновить текущую категорию
            if (ui->comboCategoryReleased->count() > 0) {
                QString newCat = ui->comboCategoryReleased->itemText(0);
                currentReleasedCategory = newCat;
                ui->listReleased->setModel(releasedModels[newCat]);
                ui->comboCategoryReleased->setCurrentText(newCat);
            } else {
                currentReleasedCategory.clear();
                ui->listReleased->setModel(nullptr);
            }
        }
    }
}

void MainWindow::loadData()
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dir.exists())
        dir.mkpath(".");

    // Загрузка вышедшего
    for (const QString& cat : releasedModels.keys()) {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + cat.toLower() + "_released.json";
        releasedItems[cat] = DataManager::loadItems(path);
        releasedModels[cat]->clear();
        for (const MediaItem& item : releasedItems[cat]) {
            releasedModels[cat]->appendRow(new QStandardItem(item.title));
        }
    }

    // Загрузка не вышедшего (UpcomingItem)
    upcomingItems = DataManager::loadUpcomingItems(upcomingPath);
    updateUpcomingModel();

    // Загрузка шлака
    trashItems = DataManager::loadItems(trashPath);
    trashModel->clear();
    for (const MediaItem& item : trashItems) {
        trashModel->appendRow(new QStandardItem(item.title));
    }
}

void MainWindow::saveData()
{
    for (const QString& cat : releasedModels.keys()) {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + cat.toLower() + "_released.json";
        QVector<MediaItem> &list = releasedItems[cat];
        list.clear();
        QStandardItemModel* model = releasedModels[cat];
        for (int i = 0; i < model->rowCount(); i++) {
            QString title = model->item(i)->text();
            MediaItem item;
            item.title = title;
            list.append(item);
        }
        DataManager::saveItems(path, list);
    }

    // Сохранение не вышедшего (UpcomingItem)
    DataManager::saveUpcomingItems(upcomingPath, upcomingItems);

    // Сохранение шлака
    trashItems.clear();
    for (int i = 0; i < trashModel->rowCount(); ++i) {
        MediaItem item;
        item.title = trashModel->item(i)->text();
        trashItems.append(item);
    }
    DataManager::saveItems(trashPath, trashItems);
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
)");
}

void MainWindow::applyLightTheme() {
    qApp->setStyleSheet("");
}

void MainWindow::setupThemeToggleButtons()
{
    QList<QPushButton*> buttons = {
        ui->btnToggleTheme
    };

    // Убедимся, что каждая кнопка checkable и стилизована
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

    // Загрузка состояния из настроек
    QSettings settings("sendy-tech", "SerialNotes");
    isDarkTheme = settings.value("darkTheme", false).toBool();

    // Применим тему до установки состояния кнопок
    if (isDarkTheme)
        applyDarkTheme();
    else
        applyLightTheme();

    // Устанавливаем начальное состояние без блокировки сигналов
    for (QPushButton* btn : buttons) {
        if (!btn) continue;
        btn->setChecked(isDarkTheme);
        btn->setText(isDarkTheme ? "🌞" : "🌙");
    }

    // Подключаем сигналы после установки состояния
    for (QPushButton* btn : buttons) {
        if (!btn) continue;

        connect(btn, &QPushButton::toggled, this, [=](bool checked) {
            isDarkTheme = checked;

            for (QPushButton* b : buttons) {
                if (!b) continue;
                b->blockSignals(true);
                b->setChecked(checked);
                b->setText(checked ? "🌞" : "🌙");
                b->blockSignals(false);
            }

            if (checked) applyDarkTheme();
            else applyLightTheme();

            QSettings settings("sendy-tech", "SerialNotes");
            settings.setValue("darkTheme", isDarkTheme);
        });
    }
}
