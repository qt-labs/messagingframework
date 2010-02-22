TEMPLATE = lib 
TARGET = qtopiamailfile 
CONFIG += qtopiamail messageserver

target.path += $$QMF_INSTALL_ROOT/plugins/messageservices

QT += network
DERFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail/build \
        -L../../../libraries/messageserver/build
macx:LIBS += -F../../../libraries/qtopiamail/build \
        -F../../../libraries/messageserver/build

HEADERS += service.h settings.h

FORMS += settings.ui

SOURCES += service.cpp settings.cpp storagelocations.cpp

include(../../../../common.pri)
