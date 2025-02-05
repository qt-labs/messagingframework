TEMPLATE = app
CONFIG += qmfclient qmfmessageserver
TARGET = tst_smtp

SRCDIR = $$PWD/../../src/plugins/messageservices/smtp
INCLUDEPATH += $$SRCDIR
QT += qmfclient qmfmessageserver qmfmessageserver-private
HEADERS += $$SRCDIR/smtpauthenticator.h \
           $$SRCDIR/smtpclient.h \
           $$SRCDIR/smtpconfiguration.h
SOURCES += $$SRCDIR/smtpclient.cpp \
           $$SRCDIR/smtpauthenticator.cpp \
           $$SRCDIR/smtpconfiguration.cpp

SOURCES += tst_smtp.cpp

include(../tests.pri)

# until we figure out how to run a test SMTP server
CONFIG += insignificant_test
