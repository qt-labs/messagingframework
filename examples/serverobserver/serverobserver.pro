TEMPLATE = app
TARGET = serverobserver5
target.path += $$[QT_INSTALL_EXAMPLES]/qmf/serverobserver
QT += widgets qmfclient qmfmessageserver

HEADERS += serverobserver.h

SOURCES += serverobserver.cpp \
	   main.cpp

include(../../common.pri)
CONFIG += install_ok
