TARGET     = QmfMessageServer
QT         = core network qmfclient qmfclient-private
CONFIG    += warn_on

MODULE_PLUGIN_TYPES = \
    messagingframework/messageservices \
    messagingframework/messagecredentials

contains(DEFINES,MESSAGESERVER_PLUGINS) {
    MODULE_PLUGIN_TYPES += messageserverplugins
    HEADERS += qmailmessageserverplugin.h
    SOURCES += qmailmessageserverplugin.cpp
}

load(qt_module)
CONFIG -= create_cmake

DEFINES += MESSAGESERVER_INTERNAL

HEADERS += \
    longstream_p.h \
    qmailauthenticator.h \
    qmailcredentials.h \
    qmailmessagebuffer.h \
    qmailmessageclassifier.h \
    qmailmessageservice.h \
    qmailserviceconfiguration.h \
    qmailstoreaccountfilter.h \
    qmailtransport.h \
    qmailheartbeattimer.h

SOURCES += \
           longstream.cpp \
           qmailauthenticator.cpp \
           qmailcredentials.cpp \
           qmailmessagebuffer.cpp \
           qmailmessageclassifier.cpp \
           qmailmessageservice.cpp \
           qmailserviceconfiguration.cpp \
           qmailstoreaccountfilter.cpp \
           qmailtransport.cpp \
           qmailheartbeattimer_qtimer.cpp # NB: There are multiple implementations

