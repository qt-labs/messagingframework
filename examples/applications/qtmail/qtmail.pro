TEMPLATE = app
TARGET = qtmail 
CONFIG += qmfutil qtopiamail messageserver
target.path += $$QMF_INSTALL_ROOT/bin

DEPENDPATH += . 

INCLUDEPATH += . ../../../src/libraries/qtopiamail \
                 ../../../src/libraries/qtopiamail/support \
                 ../../../src/libraries/qmfutil \
                 ../../../src/libraries/messageserver

LIBS += -L../../../src/libraries/qtopiamail/build \
        -L../../../src/libraries/qmfutil/build \
        -L../../../src/libraries/messageserver/build

macx:LIBS += -F../../../src/libraries/qtopiamail/build \
        -F../../../src/libraries/qmfutil/build \
        -F../../../src/libraries/messageserver/build





HEADERS += emailclient.h \
           messagelistview.h \
           searchview.h \
           selectcomposerwidget.h \
           readmail.h \
           writemail.h \
           settings/accountsettings.h \
           settings/editaccount.h \
           statusmonitorwidget.h \
           statusbar.h \
           statusmonitor.h

SOURCES += emailclient.cpp \
           main.cpp \
           messagelistview.cpp \
           searchview.cpp \
           selectcomposerwidget.cpp \
           readmail.cpp \
           writemail.cpp \
           settings/accountsettings.cpp \
           settings/editaccount.cpp \
           statusmonitorwidget.cpp \
           statusbar.cpp \
           statusmonitor.cpp

FORMS += searchviewbasephone.ui

RESOURCES += qtmail.qrc

include(../../../common.pri)
