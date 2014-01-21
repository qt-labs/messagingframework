TEMPLATE = app
TARGET = serverobserver5
QT += widgets qmfclient qmfmessageserver

target.path += $$QMF_INSTALL_ROOT/bin

HEADERS += serverobserver.h

SOURCES += serverobserver.cpp \
	   main.cpp

include(../../common.pri)
