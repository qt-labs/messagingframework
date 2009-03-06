CONFIG += qtestlib unittest
TEMPLATE = app

TARGET = tst_python_email
target.path += $$QMF_INSTALL_ROOT/tests

testdata.path = $$QMF_INSTALL_ROOT/tests/testdata 
testdata.files = testdata/

INSTALLS += target \
            testdata

BASE=$$PWD/../..
QTOPIAMAIL=$$BASE/src/libraries/qtopiamail

DEPENDPATH += .
INCLUDEPATH += . $$QTOPIAMAIL $$QTOPIAMAIL/support
LIBS += -L$$QTOPIAMAIL -lqtopiamail
QMAKE_LFLAGS += -Wl,-rpath,$$BASE/src/libraries/qtopiamail

SOURCES += tst_python_email.cpp


