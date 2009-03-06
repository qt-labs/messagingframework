CONFIG += qtestlib unittest
QT += sql
TEMPLATE = app
TARGET = tst_qmailstorekeys
target.path += $$QMF_INSTALL_ROOT/tests
INSTALLS += target

BASE=$$PWD/../..
QTOPIAMAIL=$$BASE/src/libraries/qtopiamail

DEPENDPATH += .
INCLUDEPATH += . $$QTOPIAMAIL $$QTOPIAMAIL/support
LIBS += -L$$QTOPIAMAIL -lqtopiamail
QMAKE_LFLAGS += -Wl,-rpath,$$QTOPIAMAIL

SOURCES += tst_qmailstorekeys.cpp


