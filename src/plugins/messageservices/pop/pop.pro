TEMPLATE = lib
TARGET = pop
CONFIG += plugin

target.path = $$QMF_INSTALL_ROOT/lib/qmf/plugins5/messageservices

QT = core network qmfclient qmfclient-private qmfmessageserver

HEADERS += popclient.h \
           popconfiguration.h \
           popservice.h \
           popauthenticator.h

SOURCES += popclient.cpp \
           popconfiguration.cpp \
           popservice.cpp \
           popauthenticator.cpp

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
    QT += gui widgets

HEADERS += \
           popsettings.h

FORMS += popsettings.ui

SOURCES += \
           popsettings.cpp \
}

include(../../../../common.pri)
