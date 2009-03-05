CONFIG += qtestlib
TEMPLATE = app
TARGET = tst_qmailaddress
target.path+=$$QMF_INSTALL_ROOT/tests
INSTALLS += target

DEPENDPATH += .
INCLUDEPATH += . ../../ ../../support
LIBS += -L../.. -lqtopiamail

SOURCES += tst_qmailaddress.cpp


