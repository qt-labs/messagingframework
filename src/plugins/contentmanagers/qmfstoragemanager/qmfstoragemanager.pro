TEMPLATE = lib 
TARGET = qmfstoragemanager
CONFIG += plugin
QT = core qmfclient

target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins5/contentmanagers

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

HEADERS += qmfstoragemanager.h

SOURCES += qmfstoragemanager.cpp

include(../../../../common.pri)
