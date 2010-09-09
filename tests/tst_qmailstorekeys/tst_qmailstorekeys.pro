TEMPLATE = app
CONFIG += qtestlib unittest qmfclient
TARGET = tst_qmailstorekeys

QT += sql

SOURCES += tst_qmailstorekeys.cpp

include(../tests.pri)

