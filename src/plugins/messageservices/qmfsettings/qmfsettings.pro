TEMPLATE = lib
TARGET = qmfsettings
PLUGIN_TYPE = messageservices
load(qt_plugin)

QT = core network qmfclient qmfmessageserver

HEADERS += service.h

SOURCES += service.cpp

!contains(DEFINES,QMF_NO_WIDGETS) {
    QT += gui widgets

HEADERS += settings.h

FORMS += settings.ui

SOURCES += settings.cpp storagelocations.cpp
}

