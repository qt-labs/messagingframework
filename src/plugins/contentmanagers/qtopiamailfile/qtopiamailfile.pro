TEMPLATE = lib 

include(../../../../common.pri)

TARGET = qtopiamailfilemanager 
target.path += $$QMF_INSTALL_ROOT/plugins/contentmanagers
INSTALLS += target

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail -lqtopiamail \
        -L../../../libraries/messageserver -lmessageserver

HEADERS += qtopiamailfilemanager.h

SOURCES += qtopiamailfilemanager.cpp

