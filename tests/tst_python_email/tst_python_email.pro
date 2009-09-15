CONFIG += qtestlib unittest

include(../../common.pri)

TEMPLATE = app
TARGET = tst_python_email
target.path += $$QMF_INSTALL_ROOT/tests

DEFINES += SRCDIR=\\\"$$_PRO_FILE_PWD_\\\"

testdata.path = $$QMF_INSTALL_ROOT/tests/testdata 
testdata.files = testdata/

INSTALLS += target \
            testdata

QTOPIAMAIL=../../src/libraries/qtopiamail

DEPENDPATH += .
INCLUDEPATH += . $$QTOPIAMAIL $$QTOPIAMAIL/support
LIBS += -L$$QTOPIAMAIL -lqtopiamail
QMAKE_LFLAGS += -Wl,-rpath,../../src/libraries/qtopiamail

SOURCES += tst_python_email.cpp


