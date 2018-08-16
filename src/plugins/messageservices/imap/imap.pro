TEMPLATE = lib
TARGET = imap
PLUGIN_TYPE = messageservices
load(qt_plugin)

QT = core network qmfclient qmfclient-private qmfmessageserver

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

!contains(DEFINES,QMF_NO_WIDGETS) {
    QT += qmfwidgets

    HEADERS += \
           imapsettings.h

    FORMS += imapsettings.ui

    SOURCES += \
           imapsettings.cpp

    RESOURCES += imap.qrc
}

qtConfig(system-zlib) {
    QMAKE_USE_PRIVATE += zlib
} else {
    QT_PRIVATE += zlib-private
}

