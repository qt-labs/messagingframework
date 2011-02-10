TEMPLATE = lib 
TARGET = qmfstoragemanager
CONFIG += qmfclient plugin
QT = core

target.path += $$QMF_INSTALL_ROOT/plugins/contentmanagers

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmfclient \
               ../../../libraries/qmfclient/support

LIBS += -L../../../libraries/qmfclient/build
macx:LIBS += -F../../../libraries/qmfclient/build

HEADERS += qmfstoragemanager.h

SOURCES += qmfstoragemanager.cpp

symbian: {
    load(data_caging_paths)

    TARGET.EPOCALLOWDLLDATA = 1
    TARGET.CAPABILITY = ALL -TCB

    deploy.path = C:
    pluginstub.sources = $${TARGET}.dll
    pluginstub.path = $$QT_PLUGINS_BASE_DIR/qtmail/contentmanagers
    DEPLOYMENT += pluginstub

    load(armcc_warnings)
}

include(../../../../common.pri)
