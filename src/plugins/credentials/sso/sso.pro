TEMPLATE = lib
TARGET = sso
PLUGIN_TYPE = messagecredentials
load(qt_plugin)
QT = core qmfclient qmfmessageserver

CONFIG += link_pkgconfig
PKGCONFIG += libsignon-qt5 signon-oauth2plugin

HEADERS += plugin.h \
           ssomanager.h
SOURCES += plugin.cpp \
           ssomanager.cpp

