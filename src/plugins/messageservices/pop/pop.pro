TEMPLATE = lib 
TARGET = pop 
CONFIG += qmf qmfmessageserver plugin

target.path = $$QMF_INSTALL_ROOT/plugins/messageservices

QT = core network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmf \
               ../../../libraries/qmfmessageserver \
               ../../../libraries/qmf/support

LIBS += -L../../../libraries/qmf/build \
        -L../../../libraries/qmfmessageserver/build
macx:LIBS += -F../../../libraries/qmf/build \
        -F../../../libraries/qmfmessageserver/build


HEADERS += popclient.h \
           popconfiguration.h \
           popservice.h \
           popauthenticator.h

SOURCES += popclient.cpp \
           popconfiguration.cpp \
           popservice.cpp \
           popauthenticator.cpp

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
QT += gui

HEADERS += \
           popsettings.h

FORMS += popsettings.ui

SOURCES += \
           popsettings.cpp \
}

include(../../../../common.pri)
