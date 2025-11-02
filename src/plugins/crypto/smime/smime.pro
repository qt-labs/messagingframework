TEMPLATE = lib
TARGET = smime
PLUGIN_TYPE = messagingframework/crypto
PLUGIN_CLASS_NAME = QMailCryptoSmimePlugin
load(qt_plugin)
QT = core qmfclient

HEADERS += smimeplugin.h
SOURCES += smimeplugin.cpp

include(../common.pri)
