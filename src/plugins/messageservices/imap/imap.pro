TEMPLATE = lib
TARGET = imap
PLUGIN_TYPE = messageservices
load(qt_plugin)

QT = core network qmfclient qmfclient-private qmfmessageserver qmfwidgets

contains(DEFINES,QT_QMF_USE_ALIGNEDTIMER) {
    QT += alignedtimer
}

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

#Required to build on windows
DEFINES += QMFUTIL_INTERNAL

HEADERS += \
           imapsettings.h

FORMS += imapsettings.ui

SOURCES += \
           imapsettings.cpp

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

