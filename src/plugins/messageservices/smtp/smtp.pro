TEMPLATE = lib 
TARGET = smtp 

CONFIG += qmfclient qmfmessageserver plugin
equals(QT_MAJOR_VERSION, 4) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/messageservices
    LIBS += -lqmfmessageserver -lqmfclient
}
equals(QT_MAJOR_VERSION, 5) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins5/messageservices
    LIBS += -lqmfmessageserver5 -lqmfclient5
}

QT = core network
DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmfclient \
               ../../../libraries/qmfmessageserver \
               ../../../libraries/qmfclient/support

LIBS += -L../../../libraries/qmfclient/build \
        -L../../../libraries/qmfmessageserver/build
macx:LIBS += -F../../../libraries/qmfclient/build \
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
    equals(QT_MAJOR_VERSION, 5): QT += widgets

HEADERS += \
           smtpsettings.h

FORMS += smtpsettings.ui

SOURCES += \
           smtpsettings.cpp
}

include(../../../../common.pri)
