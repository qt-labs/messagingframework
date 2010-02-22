TEMPLATE = lib 
TARGET = genericviewer 
CONFIG += qtopiamail qmfutil

target.path += $$QMF_INSTALL_ROOT/plugins/viewers

DEFINES += PLUGIN_INTERNAL

# Use webkit to render mail if available
contains(QT_CONFIG,webkit){
    QT += webkit network
    DEFINES += USE_WEBKIT
}

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmfutil \
               ../../../libraries/qtopiamail \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail/build \
        -L../../../libraries/qmfutil/build
LIBS += -F../../../libraries/qtopiamail/build \
        -F../../../libraries/qmfutil/build

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

include(../../../../common.pri)
