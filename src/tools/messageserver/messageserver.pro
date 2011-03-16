TEMPLATE = app
TARGET = messageserver
CONFIG += qmfmessageserver qmfclient
QT = core
!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR):QT += gui

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
           main.cpp \
           messageserver.cpp \
           prepareaccounts.cpp \
           newcountnotifier.cpp \
           servicehandler.cpp

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
                
symbian: {
    TARGET.CAPABILITY = ALL -TCB
    TARGET.UID3 = 0x20034928
    TARGET.EPOCHEAPSIZE = 0x20000 0x1000000
}                

include(../../../common.pri)
