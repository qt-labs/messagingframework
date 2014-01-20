SERVER_AS_DLL: {
    DEFINES += SERVER_AS_DLL
    DEFINES += QMFUTIL_INTERNAL
    TEMPLATE = lib
} else {
    TEMPLATE = app
}

equals(QT_MAJOR_VERSION, 4){
    TARGET = messageserver
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfmessageserver -framework qmfclient
    } else {
        LIBS += -lqmfmessageserver -lqmfclient
    }
}
equals(QT_MAJOR_VERSION, 5){
    TARGET = messageserver5
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfmessageserver5 -framework qmfclient5
    } else {
        LIBS += -lqmfmessageserver5 -lqmfclient5
    }
}

CONFIG += qmfmessageserver qmfclient
QT = core

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
    QT += gui
    equals(QT_MAJOR_VERSION, 5): QT += widgets
}

CONFIG -= app_bundle
target.path += $$QMF_INSTALL_ROOT/bin

DEPENDPATH += . 

INCLUDEPATH += . ../../libraries/qmfclient \
                 ../../libraries/qmfclient/support \
                 ../../libraries/qmfmessageserver

LIBS += -L../../libraries/qmfmessageserver/build \
        -L../../libraries/qmfclient/build
macx:LIBS += -F../../libraries/qmfmessageserver/build \
        -F../../libraries/qmfclient/build

HEADERS += mailmessageclient.h \
           messageserver.h \
           servicehandler.h \
           newcountnotifier.h

SOURCES += mailmessageclient.cpp \
           messageserver.cpp \
           prepareaccounts.cpp \
           newcountnotifier.cpp \
           servicehandler.cpp

!SERVER_AS_DLL: {
    SOURCES += main.cpp
}

TRANSLATIONS += messageserver-ar.ts \
                messageserver-de.ts \
                messageserver-en_GB.ts \
                messageserver-en_SU.ts \
                messageserver-en_US.ts \
                messageserver-es.ts \
                messageserver-fr.ts \
                messageserver-it.ts \
                messageserver-ja.ts \
                messageserver-ko.ts \
                messageserver-pt_BR.ts \
                messageserver-zh_CN.ts \
                messageserver-zh_TW.ts

include(../../../common.pri)
