TEMPLATE = app
TARGET = qtmail 
CONFIG += qmfutil qmfclient qmfmessageserver
target.path += $$QMF_INSTALL_ROOT/bin

DEPENDPATH += . 

INCLUDEPATH += . ../../../src/libraries/qmfclient \
                 ../../../src/libraries/qmfclient/support \
                 ../libs/qmfutil \
                 ../../../src/libraries/qmfmessageserver

LIBS += -L../../../src/libraries/qmfclient/build \
        -L../libs/qmfutil/build \
        -L../../../src/libraries/qmfmessageserver/build

macx:LIBS += -F../../../src/libraries/qmfclient/clientbuild \
        -F../libs/qmfutil/build \
        -F../../../src/libraries/qmfclient/build

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

symbian {
    TARGET.CAPABILITY += ALL -TCB
}

include(../../../common.pri)
