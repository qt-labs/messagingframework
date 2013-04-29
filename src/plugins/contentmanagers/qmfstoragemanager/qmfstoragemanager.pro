TEMPLATE = lib 
TARGET = qmfstoragemanager
CONFIG += qmfclient plugin
QT = core

equals(QT_MAJOR_VERSION, 4) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/contentmanagers
    LIBS += -lqmfclient
}
equals(QT_MAJOR_VERSION, 5) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins5/contentmanagers
    LIBS += -lqmfclient5
}

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmfclient \
               ../../../libraries/qmfclient/support

LIBS += -L../../../libraries/qmfclient/build
macx:LIBS += -F../../../libraries/qmfclient/build

HEADERS += qmfstoragemanager.h

SOURCES += qmfstoragemanager.cpp

include(../../../../common.pri)
