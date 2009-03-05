TEMPLATE = lib 

TARGET = smtp 
target.path += $$QMF_INSTALL_ROOT/plugins/messageservices
INSTALLS += target

QT += network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail -lqtopiamail \
        -L../../../libraries/messageserver -lmessageserver

HEADERS += smtpauthenticator.h \
           smtpclient.h \
           smtpconfiguration.h \
           smtpservice.h \
           smtpsettings.h

FORMS += smtpsettings.ui

SOURCES += smtpauthenticator.cpp \
           smtpclient.cpp \
           smtpconfiguration.cpp \
           smtpservice.cpp \
           smtpsettings.cpp
