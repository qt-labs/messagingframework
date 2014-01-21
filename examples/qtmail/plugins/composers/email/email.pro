TEMPLATE = lib 
TARGET = emailcomposer 
CONFIG += plugin

target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins5/composers
QT += widgets qmfclient
macx:contains(QT_CONFIG, qt_framework) {
    LIBS += -framework qmfutil5
} else {
    LIBS += -lqmfutil5
}

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libs/qmfutil

LIBS += -L../../../libs/qmfutil/build

macx:LIBS += -F../../../libs/qmfutil/build


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
