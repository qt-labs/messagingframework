TEMPLATE = lib
TARGET = pop
PLUGIN_TYPE = messagingframework/messageservices
PLUGIN_CLASS_NAME = QmfPopPlugin
load(qt_plugin)

QT = core network qmfclient qmfclient-private qmfmessageserver qmfmessageserver-private

HEADERS += popclient.h \
           popconfiguration.h \
           poplog.h \
           popservice.h \
           popauthenticator.h

SOURCES += popclient.cpp \
           popconfiguration.cpp \
           poplog.cpp \
           popservice.cpp \
           popauthenticator.cpp
