TEMPLATE = app
CONFIG += qmfclient
TARGET = tst_python_email

DEFINES += SRCDIR=\\\"$$_PRO_FILE_PWD_\\\"

equals(QT_MAJOR_VERSION, 4): testdata.path = $$QMF_INSTALL_ROOT/tests/testdata
equals(QT_MAJOR_VERSION, 5): testdata.path = $$QMF_INSTALL_ROOT/tests5/testdata

testdata.files = testdata/*

INSTALLS += testdata

SOURCES += tst_python_email.cpp

include(../tests.pri)
