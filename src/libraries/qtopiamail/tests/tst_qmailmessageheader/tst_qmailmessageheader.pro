CONFIG += qtestlib
TEMPLATE = app
TARGET = tst_qmailmessageheader
target.path += $$QMF_INSTALL_ROOT/tests
INSTALLS += target

DEPENDPATH += .
INCLUDEPATH += . ../../ ../../support
LIBS += -L../.. -lqtopiamail

SOURCES += tst_qmailmessageheader.cpp


