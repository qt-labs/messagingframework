TEMPLATE = app
CONFIG += qmfclient qmfmessageserver
TARGET = tst_imap

SRCDIR = $$PWD/../../src/plugins/messageservices/imap
INCLUDEPATH += $$SRCDIR
QT += qmfclient qmfmessageserver qmfmessageserver-private
HEADERS += $$SRCDIR/imapauthenticator.h \
           $$SRCDIR/imapclient.h \
           $$SRCDIR/imapprotocol.h \
           $$SRCDIR/imaptransport.h \
           $$SRCDIR/imapstrategy.h \
           $$SRCDIR/imapstructure.h \
           $$SRCDIR/integerregion.h \
           $$SRCDIR/imapconfiguration.h
SOURCES += $$SRCDIR/imapclient.cpp \
           $$SRCDIR/imapauthenticator.cpp \
           $$SRCDIR/imapprotocol.cpp \
           $$SRCDIR/imaptransport.cpp \
           $$SRCDIR/imapstrategy.cpp \
           $$SRCDIR/imapstructure.cpp \
           $$SRCDIR/integerregion.cpp \
           $$SRCDIR/imapconfiguration.cpp

SOURCES += tst_imap.cpp

include(../tests.pri)

# until we figure out how to run a test IMAP server
CONFIG += insignificant_test
