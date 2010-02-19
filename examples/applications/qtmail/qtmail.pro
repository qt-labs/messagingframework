TEMPLATE = app
TARGET = qtmail 
CONFIG += qmfutil qtopiamail
target.path += $$QMF_INSTALL_ROOT/bin

DEPENDPATH += . 

INCLUDEPATH += . ../../../src/libraries/qtopiamail \
                 ../../../src/libraries/qtopiamail/support \
                 ../../../src/libraries/qmfutil

LIBS += -L../../../src/libraries/qtopiamail/build \
        -L../../../src/libraries/qmfutil/build


HEADERS += emailclient.h \
           messagelistview.h \
           searchview.h \
           selectcomposerwidget.h \
           statusdisplay.h \
           readmail.h \
           writemail.h

SOURCES += emailclient.cpp \
           main.cpp \
           messagelistview.cpp \
           searchview.cpp \
           selectcomposerwidget.cpp \
           statusdisplay.cpp \
           readmail.cpp \
           writemail.cpp

FORMS += searchviewbasephone.ui

RESOURCES += qtmail.qrc

include(../../../common.pri)
