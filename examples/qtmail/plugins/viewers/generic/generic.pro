TEMPLATE = lib 
TARGET = genericviewer 
CONFIG += qmfclient qmfutil plugin

equals(QT_MAJOR_VERSION, 4) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/viewers
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfclient -framework qmfutil
    } else {
        LIBS += -lqmfclient -lqmfutil
    }
}
equals(QT_MAJOR_VERSION, 5) {
    target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins5/viewers
    QT += widgets
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfclient5 -framework qmfutil5
    } else {
        LIBS += -lqmfclient5 -lqmfutil5
    }
}

# Use webkit to render mail if available
contains(QT_CONFIG,webkit){
    QT += network
    equals(QT_MAJOR_VERSION, 5): QT += webkitwidgets
    else: QT += webkit
    DEFINES += USE_WEBKIT
}

DEPENDPATH += .

INCLUDEPATH += . ../../../libs/qmfutil \
               ../../../../../src/libraries/qmfclient \
               ../../../../../src/libraries/qmfclient/support

LIBS += -L../../../../../src/libraries/qmfclient/build \
        -L../../../libs/qmfutil/build

LIBS += -F../../../../../src/libraries/qmfclient/build \
        -F../../../libs/qmfutil/build

HEADERS += attachmentoptions.h browserwidget.h genericviewer.h

SOURCES += attachmentoptions.cpp browserwidget.cpp genericviewer.cpp

TRANSLATIONS += libgenericviewer-ar.ts \
                libgenericviewer-de.ts \
                libgenericviewer-en_GB.ts \
                libgenericviewer-en_SU.ts \
                libgenericviewer-en_US.ts \
                libgenericviewer-es.ts \
                libgenericviewer-fr.ts \
                libgenericviewer-it.ts \
                libgenericviewer-ja.ts \
                libgenericviewer-ko.ts \
                libgenericviewer-pt_BR.ts \
                libgenericviewer-zh_CN.ts \
                libgenericviewer-zh_TW.ts

include(../../../../../common.pri)
