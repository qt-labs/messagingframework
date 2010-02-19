TEMPLATE = lib 
TARGET = smtp 

CONFIG += qtopiamail messageserver

target.path += $$QMF_INSTALL_ROOT/plugins/messageservices

QT += network
DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail/build \
        -L../../../libraries/messageserver/build

HEADERS += smtpauthenticator.h \
           smtpclient.h \
           smtpconfiguration.h \
           smtpservice.h \
           smtpsettings.h

FORMS += smtpsettings.ui

SOURCES += smtpauthenticator.cpp \
           smtpclient.cpp \
           smtpconfiguration.cpp \
           smtpservice.cpp \
           smtpsettings.cpp

include(../../../../common.pri)
