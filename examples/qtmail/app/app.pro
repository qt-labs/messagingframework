TEMPLATE = app
TARGET = qtmail 
CONFIG += qmfutil qtopiamail messageserver
target.path += $$QMF_INSTALL_ROOT/bin

DEPENDPATH += . 

INCLUDEPATH += . ../../../src/libraries/qtopiamail \
                 ../../../src/libraries/qtopiamail/support \
                 ../libs/qmfutil \
                 ../../../src/libraries/messageserver

LIBS += -L../../../src/libraries/qtopiamail/build \
        -L../libs/qmfutil/build \
        -L../../../src/libraries/messageserver/build

macx:LIBS += -F../../../src/libraries/qtopiamail/build \
        -F../../libs/qmfutil/build \
        -F../../../src/libraries/messageserver/build

HEADERS += emailclient.h \
           messagelistview.h \
           searchview.h \
           selectcomposerwidget.h \
           readmail.h \
           writemail.h \
           accountsettings.h \
           editaccount.h \
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
           accountsettings.cpp \
           editaccount.cpp \
           statusmonitorwidget.cpp \
           statusbar.cpp \
           statusmonitor.cpp

FORMS += searchviewbasephone.ui

RESOURCES += qtmail.qrc

include(../../../common.pri)
