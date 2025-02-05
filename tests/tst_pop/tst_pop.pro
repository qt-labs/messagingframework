TEMPLATE = app
CONFIG += qmfclient qmfmessageserver
TARGET = tst_pop

SRCDIR = $$PWD/../../src/plugins/messageservices/pop
INCLUDEPATH += $$SRCDIR
QT += qmfclient qmfmessageserver qmfmessageserver-private
HEADERS += $$SRCDIR/popauthenticator.h \
           $$SRCDIR/popclient.h \
           $$SRCDIR/popconfiguration.h
SOURCES += $$SRCDIR/popclient.cpp \
           $$SRCDIR/popauthenticator.cpp \
           $$SRCDIR/popconfiguration.cpp

SOURCES += tst_pop.cpp

include(../tests.pri)

# until we figure out how to run a test POP server
CONFIG += insignificant_test
