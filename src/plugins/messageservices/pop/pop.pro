TEMPLATE = lib 

include(../../../../common.pri)

TARGET = pop 
target.path = $$QMF_INSTALL_ROOT/plugins/messageservices
INSTALLS += target

QT += network
DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail -lqtopiamail \
        -L../../../libraries/messageserver -lmessageserver

HEADERS += popclient.h \
           popconfiguration.h \
           popservice.h \
           popsettings.h \
           popauthenticator.h

SOURCES += popclient.cpp \
           popconfiguration.cpp \
           popservice.cpp \
           popsettings.cpp \
           popauthenticator.cpp

FORMS += popsettings.ui

