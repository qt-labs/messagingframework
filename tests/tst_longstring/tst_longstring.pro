CONFIG += qtestlib unittest
TEMPLATE = app
TARGET = tst_longstring
target.path += $$QMF_INSTALL_ROOT/tests
INSTALLS += target

QTOPIAMAIL=../../src/libraries/qtopiamail

DEPENDPATH += .
INCLUDEPATH += . $$QTOPIAMAIL $$QTOPIAMAIL/support
LIBS += -L$$QTOPIAMAIL -lqtopiamail
QMAKE_LFLAGS += -Wl,-rpath,$$QTOPIAMAIL

SOURCES += tst_longstring.cpp


