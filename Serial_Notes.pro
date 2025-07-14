QT += core gui widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    models/mediaitem.cpp \
    storage/datamanager.cpp

HEADERS += \
    mainwindow.h \
    models/mediaitem.h \
    models/upcomingitem.h \
    storage/datamanager.h

FORMS += mainwindow.ui

DISTFILES += \
    data/released.json \
    data/trash.json \
    data/upcoming.json

OTHER_FILES += \
    data/released.json \
    data/trash.json
    data/storage.js
