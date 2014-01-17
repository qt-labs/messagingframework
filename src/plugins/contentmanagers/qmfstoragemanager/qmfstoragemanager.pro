TEMPLATE = lib 
TARGET = qmfstoragemanager
CONFIG += qmfclient plugin
QT = core

equals(QT_MAJOR_VERSION, 4) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/contentmanagers
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfclient
    } else {
        LIBS += -lqmfclient
    }
}
equals(QT_MAJOR_VERSION, 5) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins5/contentmanagers
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfclient5
    } else {
        LIBS += -lqmfclient5
    }
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
