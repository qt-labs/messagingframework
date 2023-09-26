SERVER_AS_DLL: {
    DEFINES += SERVER_AS_DLL
    DEFINES += QMFUTIL_INTERNAL
    TEMPLATE = lib
} else {
    TEMPLATE = app
}

TARGET = messageserver5
QT = core dbus qmfclient qmfclient-private qmfmessageserver

contains(DEFINES, USE_HTML_PARSER) {
    QT += gui
}

!contains(DEFINES,QMF_NO_WIDGETS) {
    QT += gui widgets
}

CONFIG -= app_bundle
target.path += $$QMF_INSTALL_ROOT/bin

HEADERS += messageserver.h \
           servicehandler.h \
           newcountnotifier.h

SOURCES += messageserver.cpp \
           prepareaccounts.cpp \
           newcountnotifier.cpp \
           servicehandler.cpp

mailservice.files = ../../libraries/qmfclient/qmailservice.xml
mailservice.header_flags = -i qmailid.h -i qmailaction.h -i qmailserviceaction.h -i qmailstore.h
DBUS_ADAPTORS += mailservice

!SERVER_AS_DLL: {
    SOURCES += main.cpp
}

TRANSLATIONS += messageserver-ar.ts \
                messageserver-de.ts \
                messageserver-en_GB.ts \
                messageserver-en_SU.ts \
                messageserver-en_US.ts \
                messageserver-es.ts \
                messageserver-fr.ts \
                messageserver-it.ts \
                messageserver-ja.ts \
                messageserver-ko.ts \
                messageserver-pt_BR.ts \
                messageserver-zh_CN.ts \
                messageserver-zh_TW.ts

include(../../../common.pri)
