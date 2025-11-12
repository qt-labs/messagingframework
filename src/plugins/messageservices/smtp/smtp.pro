TEMPLATE = lib 
TARGET = smtp 
PLUGIN_TYPE = messagingframework/messageservices
PLUGIN_CLASS_NAME = QmfSmtpPlugin
load(qt_plugin)

QT = core network qmfclient qmfmessageserver

HEADERS += smtpauthenticator.h \
           smtpclient.h \
           smtpconfiguration.h \
           smtpservice.h

SOURCES += smtpauthenticator.cpp \
           smtpclient.cpp \
           smtpconfiguration.cpp \
           smtpservice.cpp
