TEMPLATE = app
TARGET = qtmail 
CONFIG += qmfutil qmf qmfmessageserver
target.path += $$QMF_INSTALL_ROOT/bin

DEPENDPATH += . 

INCLUDEPATH += . ../../../src/libraries/qmf \
                 ../../../src/libraries/qmf/support \
                 ../libs/qmfutil \
                 ../../../src/libraries/qmfmessageserver

LIBS += -L../../../src/libraries/qmf/build \
        -L../libs/qmfutil/build \
        -L../../../src/libraries/qmfmessageserver/build

macx:LIBS += -F../../../src/libraries/qmf/build \
        -F../../libs/qmfutil/build \
        -F../../../src/libraries/qmf/build

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
