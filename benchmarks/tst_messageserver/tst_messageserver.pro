TEMPLATE = app
CONFIG += unittest
QT += testlib widgets qmfclient qmfclient-private qmfmessageserver
TARGET = tst_messageserver
target.path += $$QMF_INSTALL_ROOT/tests5

DEPENDPATH += . 3rdparty

!win32 {
	DEFINES += HAVE_VALGRIND
}

IMAP_PLUGIN=$$BASE/src/plugins/messageservices/imap/
MESSAGE_SERVER=$$BASE/src/tools/messageserver

INCLUDEPATH += . 3rdparty \
                 $$IMAP_PLUGIN \
                 $$MESSAGE_SERVER 

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

linux {
	HEADERS += testmalloc.h 3rdparty/cycle_p.h
	SOURCES += testmalloc.cpp
}

INSTALLS += target
