TEMPLATE = app
CONFIG += qtestlib unittest qmf
TARGET = tst_qmailstorekeys

QT += sql

SOURCES += tst_qmailstorekeys.cpp

include(../tests.pri)

