TEMPLATE = lib
TARGET = imap
CONFIG += qmfmessageserver qmfclient plugin

equals(QT_MAJOR_VERSION, 4) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/messageservices
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfmessageserver -framework qmfclient
    } else {
        LIBS += -lqmfmessageserver -lqmfclient
    }
}
equals(QT_MAJOR_VERSION, 5) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins5/messageservices
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfmessageserver5 -framework qmfclient5
    } else {
        LIBS += -lqmfmessageserver5 -lqmfclient5
    }
}

QT = core network

contains(DEFINES,QT_QMF_USE_ALIGNEDTIMER) {
    QT += alignedtimer
}

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmfclient \
               ../../../libraries/qmfmessageserver \
               ../../../libraries/qmfclient/support

LIBS += -L../../../libraries/qmfclient/build \
        -L../../../libraries/qmfmessageserver/build \

macx:LIBS += -F../../../libraries/qmfclient/build \
        -F../../../libraries/qmfmessageserver/build \


HEADERS += imapclient.h \
           imapconfiguration.h \
           imapmailboxproperties.h \
           imapprotocol.h \
           imapservice.h \
           imapstructure.h \
           imapauthenticator.h \
           imapstrategy.h \
           integerregion.h \
           imaptransport.h \
           serviceactionqueue.h

SOURCES += imapclient.cpp \
           imapconfiguration.cpp \
           imapprotocol.cpp \
           imapservice.cpp \
           imapstructure.cpp \
           imapauthenticator.cpp \
           imapstrategy.cpp \
           integerregion.cpp \
           imaptransport.cpp \
           serviceactionqueue.cpp

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
    QT += gui
    equals(QT_MAJOR_VERSION, 5): QT += widgets

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

packagesExist(zlib) {
    CONFIG += link_pkgconfig
    PKGCONFIG += zlib
    DEFINES += QT_QMF_HAVE_ZLIB
} macx:exists( "/usr/include/zlib.h") {
    LIBS += -lz
    DEFINES += QT_QMF_HAVE_ZLIB
} else {
     warning("IMAP COMPRESS capability requires zlib")
}

include(../../../../common.pri)
