TEMPLATE = lib
TARGET = pop
PLUGIN_TYPE = messageservices
PLUGIN_CLASS_NAME = QmfPopPlugin
load(qt_plugin)

QT = core core5compat network qmfclient qmfclient-private qmfmessageserver qmfmessageserver-private

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

!contains(DEFINES,QMF_NO_WIDGETS) {
    QT += gui widgets

    HEADERS += \
           popsettings.h

    FORMS += popsettings.ui

    SOURCES += \
           popsettings.cpp \
}

