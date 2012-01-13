TEMPLATE = lib 
TARGET = qmfsettings
CONFIG += qmfclient qmfmessageserver plugin

target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/messageservices

QT = core network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmfclient \
               ../../../libraries/qmfmessageserver \
               ../../../libraries/qmfclient/support

LIBS += -L../../../libraries/qmfclient/build \
        -L../../../libraries/qmfmessageserver/build
macx:LIBS += -F../../../libraries/qmfclient/build \
        -F../../../libraries/qmfmessageserver/build

HEADERS += service.h

SOURCES += service.cpp

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
QT += gui

HEADERS += settings.h

FORMS += settings.ui

SOURCES += settings.cpp storagelocations.cpp
}

symbian: {
    load(data_caging_paths)

    TARGET.EPOCALLOWDLLDATA = 1
    TARGET.CAPABILITY = ALL -TCB
    TARGET.UID3 = 0x20034927

    deploy.path = C:
    pluginstub.sources = $${TARGET}.dll
    pluginstub.path = $$QT_PLUGINS_BASE_DIR/qtmail/messageservices
    DEPLOYMENT += pluginstub

    load(armcc_warnings)
}

include(../../../../common.pri)
