TEMPLATE = lib 

include(../../../../common.pri)

TARGET = qtopiamailfile 
target.path += $$QMF_INSTALL_ROOT/plugins/messageservices
INSTALLS += target

QT += network
DERFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail -lqtopiamail \
        -L../../../libraries/messageserver -lmessageserver

HEADERS += service.h settings.h

FORMS += settings.ui

SOURCES += service.cpp settings.cpp storagelocations.cpp

