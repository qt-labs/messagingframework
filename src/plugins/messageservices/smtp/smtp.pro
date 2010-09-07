TEMPLATE = lib 
TARGET = smtp 

CONFIG += qmf qmfmessageserver plugin

target.path += $$QMF_INSTALL_ROOT/plugins/messageservices

QT = core network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmf \
               ../../../libraries/qmfmessageserver \
               ../../../libraries/qmf/support

LIBS += -L../../../libraries/qmf/build \
        -L../../../libraries/qmfmessageserver/build
macx:LIBS += -F../../../libraries/qmf/build \
        -F../../../libraries/qmfmessageserver/build

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
