TEMPLATE = lib
TARGET = qmfsettings
CONFIG += qmfclient qmfmessageserver plugin

target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins5/messageservices
macx:contains(QT_CONFIG, qt_framework) {
    LIBS += -framework qmfclient5 -framework qmfmessageserver5
} else {
    LIBS += -lqmfclient5 -lqmfmessageserver5
}

QT = core network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmfclient \
               ../../../libraries/qmfmessageserver \
               ../../../libraries/qmfclient/support

LIBS += -L../../../libraries/qmfclient/build \
        -L../../../libraries/qmfmessageserver/build
macx:LIBS += -F../../../libraries/qmfclient/build \
        -F../../../libraries/qmfmessageserver/build

HEADERS += service.h

SOURCES += service.cpp

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
    QT += gui widgets

HEADERS += settings.h

FORMS += settings.ui

SOURCES += settings.cpp storagelocations.cpp
}

include(../../../../common.pri)
