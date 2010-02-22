TEMPLATE = lib 
CONFIG += warn_on
CONFIG += qtopiamail
TARGET = messageserver

target.path += $$QMF_INSTALL_ROOT/lib

QT *= network

symbian: {
    MMP_RULES += EXPORTUNFROZEN
}

DEFINES += MESSAGESERVER_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../qtopiamail ../qtopiamail/support

LIBS += -L../qtopiamail/build
macx:LIBS += -F../qtopiamail/build

HEADERS += qmailauthenticator.h \
           qmailmessageclassifier.h \
           qmailmessageservice.h \
           qmailserviceconfiguration.h \
           qmailstoreaccountfilter.h \
           qmailtransport.h

SOURCES += qmailauthenticator.cpp \
           qmailmessageclassifier.cpp \
           qmailmessageservice.cpp \
           qmailserviceconfiguration.cpp \
           qmailstoreaccountfilter.cpp \
           qmailtransport.cpp

include(../../../common.pri)
