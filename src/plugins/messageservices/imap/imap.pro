TEMPLATE = lib 
TARGET = imap 
CONFIG += qmfmessageserver qmf plugin

target.path += $$QMF_INSTALL_ROOT/plugins/messageservices

QT = core network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmf \
               ../../../libraries/qmfmessageserver \
               ../../../libraries/qmf/support

LIBS += -L../../../libraries/qmf/build \
        -L../../../libraries/qmfmessageserver/build \

macx:LIBS += -F../../../libraries/qmf/build \
        -F../../../libraries/qmfmessageserver/build \


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

#Required to build on windows
DEFINES += QMFUTIL_INTERNAL

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
