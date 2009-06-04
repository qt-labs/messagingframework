TEMPLATE = lib 

TARGET = messageserver
target.path += $$QMF_INSTALL_ROOT/lib
INSTALLS += target

QT *= network

CONFIG += warn_on

DEPENDPATH += .

INCLUDEPATH += . ../qtopiamail ../qtopiamail/support

LIBS += -L../qtopiamail -lqtopiamail

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

include(../../common.pri)

