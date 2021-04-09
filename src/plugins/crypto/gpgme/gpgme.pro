TEMPLATE = lib
TARGET = gpgme
PLUGIN_TYPE = crypto
PLUGIN_CLASS_NAME = QMailCryptoGpgmePlugin
load(qt_plugin)
QT = core qmfclient

HEADERS += gpgmeplugin.h
SOURCES += gpgmeplugin.cpp

include(../common.pri)
