CONFIG += qtestlib
TEMPLATE = app

TARGET = tst_python_email
target.path += $$QMF_INSTALL_ROOT/tests

testdata.path = $$QMF_INSTALL_ROOT/tests/testdata 
testdata.files = testdata/

INSTALLS += target \
            testdata

DEPENDPATH += .
INCLUDEPATH += . ../../ ../../support
LIBS += -L../.. -lqtopiamail

SOURCES += tst_python_email.cpp


