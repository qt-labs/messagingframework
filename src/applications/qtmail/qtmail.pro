TEMPLATE = app
CONFIG -= debug_and_release

TARGET = qtmail 
target.path += $$QMF_INSTALL_ROOT/bin
INSTALLS += target

DEPENDPATH += . 

INCLUDEPATH += . ../../libraries/qtopiamail \
                 ../../libraries/qtopiamail/support \
                 ../../libraries/qmfutil

LIBS += -L../../libraries/qtopiamail -lqtopiamail \
        -L../../libraries/qmfutil -lqmfutil

HEADERS += emailclient.h \
           messagelistview.h \
           searchview.h \
           selectcomposerwidget.h \
           statusdisplay_p.h \
           readmail.h \
           writemail.h \
           quicksearchwidget.h

SOURCES += emailclient.cpp \
           main.cpp \
           messagelistview.cpp \
           searchview.cpp \
           selectcomposerwidget.cpp \
           statusdisplay.cpp \
           readmail.cpp \
           writemail.cpp \
           quicksearchwidget.cpp

FORMS += searchviewbasephone.ui

RESOURCES += qtmail.qrc

include(../../common.pri)

