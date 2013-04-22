TEMPLATE = app
CONFIG += unittest
CONFIG += qmfmessageserver qmfclient
QT += testlib
TARGET = tst_messageserver

equals(QT_MAJOR_VERSION, 4): target.path += $$QMF_INSTALL_ROOT/tests
equals(QT_MAJOR_VERSION, 5){
    target.path += $$QMF_INSTALL_ROOT/tests5
    QT += widgets
}

BASE=../../
include($$BASE/common.pri)


DEPENDPATH += . 3rdparty

DEFINES += PLUGIN_STATIC_LINK

!win32 {
	DEFINES += HAVE_VALGRIND
}

IMAP_PLUGIN=$$BASE/src/plugins/messageservices/imap/
MESSAGE_SERVER=$$BASE/src/tools/messageserver

INCLUDEPATH += . 3rdparty $$BASE/src/libraries/qmfclient \
                 $$BASE/src/libraries/qmfclient/support \
                 $$BASE/src/libraries/qmfmessageserver \
                 $$IMAP_PLUGIN \
                 $$MESSAGE_SERVER 

LIBS += -L$$BASE/src/libraries/qmfmessageserver/build \
        -L$$BASE/src/libraries/qmfclient/build

equals(QT_MAJOR_VERSION, 4): LIBS += -lqmfmessageserver -lqmfclient
equals(QT_MAJOR_VERSION, 5): LIBS += -lqmfmessageserver5 -lqmfclient5

QMAKE_LFLAGS += -Wl,-rpath,$$BASE/src/libraries/qmfclient \
    -Wl,-rpath,$$BASE/src/libraries/qmfmessageserver

HEADERS += benchmarkcontext.h \
           qscopedconnection.h \
           testfsusage.h \
           $$IMAP_PLUGIN/imapconfiguration.h \
           $$MESSAGE_SERVER/mailmessageclient.h \
           $$MESSAGE_SERVER/messageserver.h \
           $$MESSAGE_SERVER/servicehandler.h \
           $$MESSAGE_SERVER/newcountnotifier.h

SOURCES += benchmarkcontext.cpp \
           qscopedconnection.cpp \
           testfsusage.cpp \
           tst_messageserver.cpp \
           $$IMAP_PLUGIN/imapconfiguration.cpp \
           $$MESSAGE_SERVER/mailmessageclient.cpp \
           $$MESSAGE_SERVER/messageserver.cpp \
           $$MESSAGE_SERVER/prepareaccounts.cpp \
           $$MESSAGE_SERVER/servicehandler.cpp \
           $$MESSAGE_SERVER/newcountnotifier.cpp

!win32 {
	HEADERS += testmalloc.h 3rdparty/cycle_p.h
	SOURCES += testmalloc.cpp
}


