TEMPLATE = lib 
TARGET = smtp 

CONFIG += qtopiamail messageserver plugin

target.path += $$QMF_INSTALL_ROOT/plugins/messageservices

QT = core network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail/build \
        -L../../../libraries/messageserver/build
macx:LIBS += -F../../../libraries/qtopiamail/build \
        -F../../../libraries/messageserver/build

HEADERS += smtpauthenticator.h \
           smtpclient.h \
           smtpconfiguration.h \
           smtpservice.h

SOURCES += smtpauthenticator.cpp \
           smtpclient.cpp \
           smtpconfiguration.cpp \
           smtpservice.cpp

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
QT += gui

HEADERS += \
           smtpsettings.h

FORMS += smtpsettings.ui

SOURCES += \
           smtpsettings.cpp
}

include(../../../../common.pri)
