TEMPLATE = app

include(../../../common.pri)

TARGET = qtmail 
target.path += $$QMF_INSTALL_ROOT/bin
INSTALLS += target

DEPENDPATH += . 

INCLUDEPATH += . ../../../src/libraries/qtopiamail \
                 ../../../src/libraries/qtopiamail/support \
                 ../../../src/libraries/qmfutil

LIBS += -L../../../src/libraries/qtopiamail -lqtopiamail \
        -L../../../src/libraries/qmfutil -lqmfutil

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

