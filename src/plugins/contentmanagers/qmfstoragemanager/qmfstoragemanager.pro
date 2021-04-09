TEMPLATE = lib 
TARGET = qmfstoragemanager
PLUGIN_TYPE = contentmanagers
PLUGIN_CLASS_NAME = QmfStorageManagerPlugin
load(qt_plugin)
QT = core qmfclient

DEFINES += PLUGIN_INTERNAL

HEADERS += qmfstoragemanager.h

SOURCES += qmfstoragemanager.cpp
