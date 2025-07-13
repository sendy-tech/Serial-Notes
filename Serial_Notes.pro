QT += core gui widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    models/mediaitem.cpp \
    models/releasedmodel.cpp \
    models/trashmodel.cpp \
    storage/datamanager.cpp

HEADERS += \
    mainwindow.h \
    models/mediaitem.h \
    models/releasedmodel.h \
    models/trashmodel.h \
    models/upcomingitem.h \
    storage/datamanager.h

FORMS += mainwindow.ui
