TEMPLATE = app
CONFIG += qtestlib unittest qmfclient
TARGET = tst_qmailstore

QT += sql

SOURCES += tst_qmailstore.cpp

include(../tests.pri)

