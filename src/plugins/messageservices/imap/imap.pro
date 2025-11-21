TEMPLATE = lib
TARGET = imap
PLUGIN_TYPE = messagingframework/messageservices
PLUGIN_CLASS_NAME = QmfImapPlugin
load(qt_plugin)

QT = core network qmfclient qmfclient-private qmfmessageserver qmfmessageserver-private

HEADERS += imapclient.h \
           imapconfiguration.h \
           imaplog.h \
           imapmailboxproperties.h \
           imapprotocol.h \
           imapservice.h \
           imapstructure.h \
           imapauthenticator.h \
           imapstrategy.h \
           integerregion.h \
           imaptransport.h \
           serviceactionqueue.h \
           idlenetworksession.h

SOURCES += imapclient.cpp \
           imapconfiguration.cpp \
           imaplog.cpp \
           imapprotocol.cpp \
           imapservice.cpp \
           imapstructure.cpp \
           imapauthenticator.cpp \
           imapstrategy.cpp \
           integerregion.cpp \
           imaptransport.cpp \
           serviceactionqueue.cpp \
           idlenetworksession.cpp

qtConfig(system-zlib) {
    QMAKE_USE_PRIVATE += zlib
} else {
    QT_PRIVATE += zlib-private
}

# hack longstream work without the private/ on include
INCLUDEPATH += $$PWD/../../../libraries/qmfmessageserver/
