TEMPLATE = app
CONFIG += qtestlib unittest qtopiamail
TARGET = tst_python_email

target.path += $$QMF_INSTALL_ROOT/tests

DEFINES += SRCDIR=\\\"$$_PRO_FILE_PWD_\\\"

testdata.path = $$QMF_INSTALL_ROOT/tests/testdata 
testdata.files = testdata/

INSTALLS += testdata

QTOPIAMAIL=../../src/libraries/qtopiamail

DEPENDPATH += .
INCLUDEPATH += . $$QTOPIAMAIL $$QTOPIAMAIL/support
LIBS += -L$$QTOPIAMAIL/build
QMAKE_LFLAGS += -Wl,-rpath,$$QTOPIAMAIL

SOURCES += tst_python_email.cpp


include(../../common.pri)
