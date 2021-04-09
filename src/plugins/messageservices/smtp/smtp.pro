TEMPLATE = lib 
TARGET = smtp 
PLUGIN_TYPE = messageservices
PLUGIN_CLASS_NAME = QmfSmtpPlugin
load(qt_plugin)

QT = core core5compat network qmfclient qmfmessageserver

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

