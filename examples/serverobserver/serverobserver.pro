TEMPLATE = app
TARGET = serverobserver
target.path += $$[QT_INSTALL_EXAMPLES]/qmf/serverobserver
QT += widgets qmfclient qmfmessageserver

HEADERS += serverobserver.h

SOURCES += serverobserver.cpp \
	   main.cpp

INSTALLS += target

CONFIG += install_ok
