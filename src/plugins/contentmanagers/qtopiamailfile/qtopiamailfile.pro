TEMPLATE = lib 
CONFIG -= debug_and_release

TARGET = qtopiamailfilemanager 
target.path += $$QMF_INSTALL_ROOT/plugins/contentmanagers
INSTALLS += target

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/messageserver \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail -lqtopiamail \
        -L../../../libraries/messageserver -lmessageserver

HEADERS += qtopiamailfilemanager.h

SOURCES += qtopiamailfilemanager.cpp
