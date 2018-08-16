TEMPLATE = lib
TARGET = pop
PLUGIN_TYPE = messageservices
load(qt_plugin)

QT = core network qmfclient qmfclient-private qmfmessageserver

HEADERS += popclient.h \
           popconfiguration.h \
           popservice.h \
           popauthenticator.h

SOURCES += popclient.cpp \
           popconfiguration.cpp \
           popservice.cpp \
           popauthenticator.cpp

!contains(DEFINES,QMF_NO_WIDGETS) {
    QT += gui widgets

    HEADERS += \
           popsettings.h

    FORMS += popsettings.ui

    SOURCES += \
           popsettings.cpp \
}

