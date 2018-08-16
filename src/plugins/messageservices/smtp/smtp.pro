TEMPLATE = lib 
TARGET = smtp 
PLUGIN_TYPE = messageservices
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

!contains(DEFINES,QMF_NO_WIDGETS) {
    QT += gui widgets qmfwidgets

    HEADERS += \
           smtpsettings.h

    FORMS += smtpsettings.ui

    SOURCES += \
           smtpsettings.cpp
}

