TEMPLATE = app

TARGET = messageserver5
QT = core dbus qmfclient qmfclient-private qmfmessageserver qmfmessageserver-private

contains(DEFINES, USE_HTML_PARSER) {
    QT += gui
}

!contains(DEFINES,QMF_NO_WIDGETS) {
    QT += gui widgets
}

CONFIG -= app_bundle
target.path += $$QMF_INSTALL_ROOT/bin

HEADERS += messageserver.h \
           servicehandler.h

SOURCES += messageserver.cpp \
           servicehandler.cpp

mailservice.files = ../../libraries/qmfclient/qmailservice.xml
mailservice.header_flags = -i qmailid.h -i qmailaction.h -i qmailserviceaction.h -i qmailstore.h
DBUS_ADAPTORS += mailservice

SOURCES += main.cpp

TRANSLATIONS += messageserver-de.ts \
                messageserver-en_US.ts

INSTALLS += target
