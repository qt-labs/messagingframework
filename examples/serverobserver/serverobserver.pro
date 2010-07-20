TEMPLATE = app
TARGET = serverobserver
target.path += $$QMF_INSTALL_ROOT/bin
CONFIG += qtopiamail messageserver

DEPENDPATH += .
INCLUDEPATH += . ../../src/libraries/qtopiamail \
                 ../../src/libraries/qtopiamail/support \
                 ../../src/libraries/messageserver \

LIBS += -L../../src/libraries/qtopiamail/build \
        -L../../src/libraries/messageserver/build

macx:LIBS += -F../../src/libraries/qtopiamail/build \
        -F../../src/libraries/messageserver/build

HEADERS += serverobserver.h

SOURCES += serverobserver.cpp \
	   main.cpp

include(../../common.pri)
