TEMPLATE = app
TARGET = serverobserver
target.path += $$QMF_INSTALL_ROOT/bin
CONFIG += qmf qmfmessageserver

DEPENDPATH += .
INCLUDEPATH += . ../../src/libraries/qmf \
                 ../../src/libraries/qmf/support \
                 ../../src/libraries/qmfmessageserver \

LIBS += -L../../src/libraries/qmf/build \
        -L../../src/libraries/qmfmessageserver/build

macx:LIBS += -F../../src/libraries/qmf/build \
        -F../../src/libraries/qmfmessageserver/build

HEADERS += serverobserver.h

SOURCES += serverobserver.cpp \
	   main.cpp

include(../../common.pri)
