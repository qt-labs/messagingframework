TEMPLATE = app
CONFIG += qtestlib unittest qtopiamail
TARGET = tst_qmailstorekeys

QT += sql

SOURCES += tst_qmailstorekeys.cpp

include(../tests.pri)

