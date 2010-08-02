TEMPLATE = lib 
TARGET = pop 
CONFIG += qtopiamail messageserver plugin

target.path = $$QMF_INSTALL_ROOT/plugins/messageservices

QT = core network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail/build \
        -L../../../libraries/messageserver/build
macx:LIBS += -F../../../libraries/qtopiamail/build \
        -F../../../libraries/messageserver/build


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
