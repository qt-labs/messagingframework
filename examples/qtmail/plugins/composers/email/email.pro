TEMPLATE = lib 
TARGET = emailcomposer 
CONFIG += qmfclient qmfutil plugin

QT += widgets


target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/composers

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libs/qmfutil \
               ../../../../../src/libraries/qmfclient \
               ../../../../../src/libraries/qmfclient/support

LIBS += -L../../../../../src/libraries/qmfclient/build \
        -L../../../libs/qmfutil/build

macx:LIBS += -F../../../../../src/libraries/qmfclient/build \
        -F../../../libs/qmfutil/build


HEADERS += emailcomposer.h \
           attachmentlistwidget.h

SOURCES += emailcomposer.cpp \
           attachmentlistwidget.cpp

TRANSLATIONS += libemailcomposer-ar.ts \
                libemailcomposer-de.ts \
                libemailcomposer-en_GB.ts \
                libemailcomposer-en_SU.ts \
                libemailcomposer-en_US.ts \
                libemailcomposer-es.ts \
                libemailcomposer-fr.ts \
                libemailcomposer-it.ts \
                libemailcomposer-ja.ts \
                libemailcomposer-ko.ts \
                libemailcomposer-pt_BR.ts \
                libemailcomposer-zh_CN.ts \
                libemailcomposer-zh_TW.ts

RESOURCES += email.qrc                

include(../../../../../common.pri)
