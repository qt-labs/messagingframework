TEMPLATE = lib 
TARGET = qmfstoragemanager
CONFIG += qmf plugin
QT = core

target.path += $$QMF_INSTALL_ROOT/plugins/contentmanagers

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmf \
               ../../../libraries/qmf/support

LIBS += -L../../../libraries/qmf/build
macx:LIBS += -F../../../libraries/qmf/build

HEADERS += qmfstoragemanager.h

SOURCES += qmfstoragemanager.cpp

include(../../../../common.pri)
