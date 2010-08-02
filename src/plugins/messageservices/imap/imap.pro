TEMPLATE = lib 
TARGET = imap 
CONFIG += messageserver qtopiamail plugin

target.path += $$QMF_INSTALL_ROOT/plugins/messageservices

QT = core network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail/build \
        -L../../../libraries/messageserver/build \

macx:LIBS += -F../../../libraries/qtopiamail/build \
        -F../../../libraries/messageserver/build \


HEADERS += imapclient.h \
           imapconfiguration.h \
           imapmailboxproperties.h \
           imapprotocol.h \
           imapservice.h \
           imapstructure.h \
           imapauthenticator.h \
           imapstrategy.h \
           integerregion.h

SOURCES += imapclient.cpp \
           imapconfiguration.cpp \
           imapprotocol.cpp \
           imapservice.cpp \
           imapstructure.cpp \
           imapauthenticator.cpp \
           imapstrategy.cpp \
           integerregion.cpp

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
QT += gui

QMFUTIL_LIB = ../../../../examples/qtmail/libs/qmfutil

INCLUDEPATH += \
               $$QMFUTIL_LIB
HEADERS += \
           imapsettings.h \
           $$QMFUTIL_LIB/selectfolder.h \
           $$QMFUTIL_LIB/emailfoldermodel.h \
           $$QMFUTIL_LIB/foldermodel.h \
           $$QMFUTIL_LIB/folderview.h \
           $$QMFUTIL_LIB/folderdelegate.h \
           $$QMFUTIL_LIB/emailfolderview.h \
           $$QMFUTIL_LIB/qtmailnamespace.h

FORMS += imapsettings.ui

SOURCES += \
           imapsettings.cpp \
           $$QMFUTIL_LIB/selectfolder.cpp \
           $$QMFUTIL_LIB/emailfoldermodel.cpp \
           $$QMFUTIL_LIB/foldermodel.cpp \
           $$QMFUTIL_LIB/folderview.cpp \
           $$QMFUTIL_LIB/folderdelegate.cpp \
           $$QMFUTIL_LIB/emailfolderview.cpp \
           $$QMFUTIL_LIB/qtmailnamespace.cpp

RESOURCES += imap.qrc                
}

include(../../../../common.pri)
