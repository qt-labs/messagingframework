TEMPLATE = app
QT += network
TARGET = tst_qcop
CONFIG += unitest qmfclient

SOURCES += tst_qcop.cpp

include(../tests.pri)
