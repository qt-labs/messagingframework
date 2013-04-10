TEMPLATE = app
TARGET = serverobserver
target.path += $$QMF_INSTALL_ROOT/bin
CONFIG += qmfclient qmfmessageserver

equals(QT_MAJOR_VERSION, 5): QT += widgets

DEPENDPATH += .
INCLUDEPATH += . ../../src/libraries/qmfclient \
                 ../../src/libraries/qmfclient/support \
                 ../../src/libraries/qmfmessageserver \

LIBS += -L../../src/libraries/qmfclient/build \
        -L../../src/libraries/qmfmessageserver/build

macx:LIBS += -F../../src/libraries/qmfclient/build \
        -F../../src/libraries/qmfmessageserver/build

LIBS += -lqmfmessageserver -lqmfclient

HEADERS += serverobserver.h

SOURCES += serverobserver.cpp \
	   main.cpp

include(../../common.pri)
