TARGET     = QmfMessageServer
QT         = core network qmfclient qmfclient-private
CONFIG    += warn_on

MODULE_PLUGIN_TYPES = \
    messageservices

contains(DEFINES,MESSAGESERVER_PLUGINS) {
    MODULE_PLUGIN_TYPES += messageserverplugins
    HEADERS += qmailmessageserverplugin.h
    SOURCES += qmailmessageserverplugin.cpp
}

load(qt_module)
CONFIG -= create_cmake

!contains(DEFINES,QMF_NO_WIDGETS) {
    QT += gui widgets
}

DEFINES += MESSAGESERVER_INTERNAL

HEADERS += \
    qmailauthenticator.h \
    qmailmessagebuffer.h \
    qmailmessageclassifier.h \
    qmailmessageservice.h \
    qmailserviceconfiguration.h \
    qmailstoreaccountfilter.h \
    qmailtransport.h \
    qmailheartbeattimer.h

SOURCES += qmailauthenticator.cpp \
           qmailmessagebuffer.cpp \
           qmailmessageclassifier.cpp \
           qmailmessageservice.cpp \
           qmailserviceconfiguration.cpp \
           qmailstoreaccountfilter.cpp \
           qmailtransport.cpp \
           qmailheartbeattimer_qtimer.cpp # NB: There are multiple implementations

