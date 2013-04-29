TEMPLATE = lib 
TARGET = emailcomposer 
CONFIG += qmfclient qmfutil plugin

equals(QT_MAJOR_VERSION, 4) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/composers
    LIBS += -lqmfclient -lqmfutil
}
equals(QT_MAJOR_VERSION, 5) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins5/composers
    QT += widgets
    LIBS += -lqmfclient5 -lqmfutil5
}

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
