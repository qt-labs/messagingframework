CONFIG += qtestlib unittest
TEMPLATE = app

TARGET = tst_python_email
target.path += $$QMF_INSTALL_ROOT/tests

DEFINES += \'_STR(X)=\\$${LITERAL_HASH}X\'
DEFINES += \'STR(X)=_STR(X)\'
DEFINES += \'_SRCDIR=$$PWD\'
DEFINES += \'SRCDIR=STR(_SRCDIR)\'

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


