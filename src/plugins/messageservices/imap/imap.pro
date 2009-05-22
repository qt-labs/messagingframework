TEMPLATE = lib 
CONFIG -= debug_and_release

TARGET = imap 
target.path += $$QMF_INSTALL_ROOT/plugins/messageservices
INSTALLS += target

QT += network
DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail -lqtopiamail \
        -L../../../libraries/messageserver -lmessageserver

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
