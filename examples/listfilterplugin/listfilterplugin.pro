TEMPLATE = lib
TARGET = listfilterplugin
CONFIG += qtopiamail plugin
QT = core

target.path += $$QMF_INSTALL_ROOT/plugins/contentmanagers

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../src/libraries/qtopiamail \
               ../../src/libraries/qtopiamail/support

LIBS += -L../../src/libraries/qtopiamail/build
macx:LIBS += -F../../src/libraries/qtopiamail/build

HEADERS += listfilterplugin.h

SOURCES += listfilterplugin.cpp

include(../../common.pri)
