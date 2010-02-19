TEMPLATE = app
CONFIG += qtestlib unittest qtopiamail
TARGET = tst_qmailstore

QT += sql

SOURCES += tst_qmailstore.cpp

include(../tests.pri)

