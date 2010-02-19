CONFIG += qtestlib unittest
CONFIG += qtopiamail


TEMPLATE = app
TARGET = tst_longstring
target.path += $$QMF_INSTALL_ROOT/tests

QTOPIAMAIL=../../src/libraries/qtopiamail

DEPENDPATH += .
INCLUDEPATH += . $$QTOPIAMAIL $$QTOPIAMAIL/support
LIBS += -L$$QTOPIAMAIL/build
QMAKE_LFLAGS += -Wl,-rpath,$$QTOPIAMAIL

SOURCES += tst_longstring.cpp

include(../../common.pri)
