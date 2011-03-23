TEMPLATE = lib 
TARGET = genericviewer 
CONFIG += qmfclient qmfutil plugin

target.path += $$QMF_INSTALL_ROOT/plugins/viewers

# Use webkit to render mail if available
contains(QT_CONFIG,webkit){
    QT += webkit network
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

symbian: {
    load(data_caging_paths)

    TARGET.EPOCALLOWDLLDATA = 1
    TARGET.CAPABILITY = ALL -TCB

    deploy.path = C:
    pluginstub.sources = $${TARGET}.dll
    pluginstub.path = $$QT_PLUGINS_BASE_DIR/qtmail/viewers
    DEPLOYMENT += pluginstub

    load(armcc_warnings)
}

include(../../../../../common.pri)
