CONFIG += qtestlib
TEMPLATE = app
TARGET = tst_qmailmessagepart
target.path += $$QMF_INSTALL_ROOT/tests
INSTALLS += target

DEPENDPATH += .
INCLUDEPATH += . ../../ ../../support
LIBS += -L../.. -lqtopiamail

SOURCES += tst_qmailmessagepart.cpp


