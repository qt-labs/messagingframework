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

macx:LIBS += -F../../../src/libraries/qmfclient/build \
             -F../libs/qmfutil/build \
             -F../../../src/libraries/qmfmessageserver/build

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
    include(../../../symbianoptions.pri)

    TARGET.CAPABILITY += ALL -TCB
    SERVER_AS_DLL: {
        DEFINES += SERVER_AS_DLL
        INCLUDEPATH += ../../../src/tools/messageserver
        LIBS += -lmessageserver
    }
}

include(../../../common.pri)
