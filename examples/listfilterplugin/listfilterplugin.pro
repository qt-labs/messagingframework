TEMPLATE = lib
TARGET = listfilterplugin
CONFIG += qmfclient plugin install_ok
QT = core

target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/contentmanagers

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../src/libraries/qmfclient \
               ../../src/libraries/qmfclient/support

LIBS += -L../../src/libraries/qmfclient/build
macx:LIBS += -F../../src/libraries/qmfclient/build

HEADERS += listfilterplugin.h

SOURCES += listfilterplugin.cpp

include(../../common.pri)
