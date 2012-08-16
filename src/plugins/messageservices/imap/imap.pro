TEMPLATE = lib 
TARGET = imap 
CONFIG += qmfmessageserver qmfclient plugin

target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/messageservices

QT = core network alignedtimer

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
QT += gui widgets

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

symbian: {
    load(data_caging_paths)

    TARGET.EPOCALLOWDLLDATA = 1
    TARGET.CAPABILITY = ALL -TCB
    TARGET.UID3 = 0x20034924

    deploy.path = C:
    pluginstub.sources = $${TARGET}.dll
    pluginstub.path = $$QT_PLUGINS_BASE_DIR/qtmail/messageservices
    DEPLOYMENT += pluginstub

    load(armcc_warnings)
}

packagesExist(zlib) {
    CONFIG += link_pkgconfig
    PKGCONFIG += zlib
    DEFINES += QT_QMF_HAVE_ZLIB
} else {
     warning("IMAP COMPRESS capability requires zlib")
}

include(../../../../common.pri)
