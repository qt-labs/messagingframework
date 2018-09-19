TEMPLATE = app
CONFIG += qmfclient
TARGET = tst_crypto

SOURCES += tst_crypto.cpp

LIBS += $$system(gpgme-config --libs)

include(../tests.pri)
