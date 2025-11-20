TARGET     = QmfMessageServer
QT         = core network qmfclient qmfclient-private
CONFIG    += warn_on

MODULE_PLUGIN_TYPES = \
    messagingframework/messageservices \
    messagingframework/messagecredentials \
    messagingframework/messageserverplugins

load(qt_module)
CONFIG -= create_cmake

DEFINES += MESSAGESERVER_INTERNAL

HEADERS += \
    longstream_p.h \
    qmailauthenticator.h \
    qmailcredentials.h \
    qmailmessagebuffer.h \
    qmailmessageclassifier.h \
    qmailmessageserverplugin.h \
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
           qmailmessageserverplugin.cpp \
           qmailmessageservice.cpp \
           qmailserviceconfiguration.cpp \
           qmailstoreaccountfilter.cpp \
           qmailtransport.cpp \
           qmailheartbeattimer_qtimer.cpp # NB: There are multiple implementations

