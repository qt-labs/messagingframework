TEMPLATE = app
CONFIG += qmfclient
TARGET = tst_python_email

DEFINES += SRCDIR=\\\"$$_PRO_FILE_PWD_\\\"

testdata.path = $$QMF_INSTALL_ROOT/tests/testdata
testdata.files = testdata/*

INSTALLS += testdata

SOURCES += tst_python_email.cpp

include(../tests.pri)
