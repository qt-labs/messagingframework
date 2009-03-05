CONFIG += qtestlib
QT += sql
TEMPLATE = app
TARGET = tst_qmailstorekeys
target.path += $$QMF_INSTALL_ROOT/tests
INSTALLS += target

DEPENDPATH += .
INCLUDEPATH += . ../../ ../../support
LIBS += -L../.. -lqtopiamail

SOURCES += tst_qmailstorekeys.cpp


