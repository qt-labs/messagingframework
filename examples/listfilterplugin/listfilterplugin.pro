TEMPLATE = lib
TARGET = listfilterplugin
CONFIG += qmf plugin
QT = core

target.path += $$QMF_INSTALL_ROOT/plugins/contentmanagers

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../src/libraries/qmf \
               ../../src/libraries/qmf/support

LIBS += -L../../src/libraries/qmf/build
macx:LIBS += -F../../src/libraries/qmf/build

HEADERS += listfilterplugin.h

SOURCES += listfilterplugin.cpp

include(../../common.pri)
