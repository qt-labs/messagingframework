CONFIG += qtestlib unittest
CONFIG -= debug_and_release
TEMPLATE = app
TARGET = tst_qmailaddress
target.path+=$$QMF_INSTALL_ROOT/tests
INSTALLS += target

QTOPIAMAIL=../../src/libraries/qtopiamail

DEPENDPATH += .
INCLUDEPATH += . $$QTOPIAMAIL $$QTOPIAMAIL/support
LIBS += -L$$QTOPIAMAIL -lqtopiamail
QMAKE_LFLAGS += -Wl,-rpath,$$QTOPIAMAIL

SOURCES += tst_qmailaddress.cpp


