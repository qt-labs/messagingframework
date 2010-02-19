TEMPLATE = lib 
TARGET = qtopiamailfilemanager 
CONFIG += qtopiamail

target.path += $$QMF_INSTALL_ROOT/plugins/contentmanagers

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qtopiamail \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail/build

HEADERS += qtopiamailfilemanager.h

SOURCES += qtopiamailfilemanager.cpp

include(../../../../common.pri)
