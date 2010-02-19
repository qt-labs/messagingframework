TEMPLATE = lib 
TARGET = pop 
CONFIG += qtopiamail messageserver

target.path = $$QMF_INSTALL_ROOT/plugins/messageservices

QT += network
DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail/build \
        -L../../../libraries/messageserver/build

HEADERS += popclient.h \
           popconfiguration.h \
           popservice.h \
           popsettings.h \
           popauthenticator.h

SOURCES += popclient.cpp \
           popconfiguration.cpp \
           popservice.cpp \
           popsettings.cpp \
           popauthenticator.cpp

FORMS += popsettings.ui

include(../../../../common.pri)
