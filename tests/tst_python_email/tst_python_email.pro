TEMPLATE = app
CONFIG += qtestlib unittest qmfclient
TARGET = tst_python_email

!symbian {
    DEFINES += SRCDIR=\\\"$$_PRO_FILE_PWD_\\\"

    testdata.path = $$QMF_INSTALL_ROOT/tests/testdata
    testdata.files = testdata/

    INSTALLS += testdata
}

symbian {
    testdata.sources = testdata/*
    testdata.path = testdata

    DEPLOYMENT += testdata
}

SOURCES += tst_python_email.cpp

include(../tests.pri)
