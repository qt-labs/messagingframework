TEMPLATE = lib 

include(../../../../common.pri)

TARGET = qtopiamailfilemanager 
target.path += $$QMF_INSTALL_ROOT/plugins/contentmanagers
INSTALLS += target

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail -lqtopiamail

HEADERS += qtopiamailfilemanager.h

SOURCES += qtopiamailfilemanager.cpp

