TEMPLATE = lib 
TARGET = imap 
CONFIG += messageserver qtopiamail qmfutil


target.path += $$QMF_INSTALL_ROOT/plugins/messageservices

QT += network
DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support \
               ../../../libraries/qmfutil

LIBS += -L../../../libraries/qtopiamail/build \
        -L../../../libraries/messageserver/build \
        -L../../../libraries/qmfutil/build

HEADERS += imapclient.h \
           imapconfiguration.h \
           imapmailboxproperties.h \
           imapprotocol.h \
           imapservice.h \
           imapsettings.h \
           imapstructure.h \
           imapauthenticator.h \
           imapstrategy.h \
           integerregion.h

FORMS += imapsettings.ui

SOURCES += imapclient.cpp \
           imapconfiguration.cpp \
           imapprotocol.cpp \
           imapservice.cpp \
           imapsettings.cpp \
           imapstructure.cpp \
           imapauthenticator.cpp \
           imapstrategy.cpp \
           integerregion.cpp

RESOURCES += imap.qrc                

include(../../../../common.pri)
