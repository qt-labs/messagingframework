TEMPLATE = lib 
TARGET = qmfsettings
CONFIG += qmf qmfmessageserver plugin

target.path += $$QMF_INSTALL_ROOT/plugins/messageservices

QT = core network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmf \
               ../../../libraries/qmfmessageserver \
               ../../../libraries/qmf/support

LIBS += -L../../../libraries/qmf/build \
        -L../../../libraries/qmfmessageserver/build
macx:LIBS += -F../../../libraries/qmf/build \
        -F../../../libraries/qmfmessageserver/build

HEADERS += service.h

SOURCES += service.cpp

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
QT += gui

HEADERS += settings.h

FORMS += settings.ui

SOURCES += settings.cpp storagelocations.cpp
}

include(../../../../common.pri)
