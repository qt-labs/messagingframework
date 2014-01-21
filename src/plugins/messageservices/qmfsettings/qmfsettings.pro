TEMPLATE = lib
TARGET = qmfsettings
CONFIG += plugin

target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins5/messageservices

QT = core network qmfclient qmfmessageserver

HEADERS += service.h

SOURCES += service.cpp

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
    QT += gui widgets

HEADERS += settings.h

FORMS += settings.ui

SOURCES += settings.cpp storagelocations.cpp
}

include(../../../../common.pri)
