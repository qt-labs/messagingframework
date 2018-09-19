TEMPLATE = lib
TARGET = smime
PLUGIN_TYPE = crypto
load(qt_plugin)
QT = core qmfclient

HEADERS += smimeplugin.h
SOURCES += smimeplugin.cpp

include(../common.pri)
