TEMPLATE = lib 
TARGET = qmfstoragemanager
CONFIG += qmfclient plugin
QT = core

target.path += $$QMF_INSTALL_ROOT/lib/qmf/plugins/contentmanagers

DEFINES += PLUGIN_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmfclient \
               ../../../libraries/qmfclient/support

LIBS += -L../../../libraries/qmfclient/build
macx:LIBS += -F../../../libraries/qmfclient/build

HEADERS += qmfstoragemanager.h

SOURCES += qmfstoragemanager.cpp

symbian: {
    include(../../../../symbianoptions.pri)

    contains(CONFIG, SYMBIAN_USE_DATA_CAGED_FILES) {
        DEFINES += SYMBIAN_USE_DATA_CAGED_FILES

        INCLUDEPATH += $$APP_LAYER_SYSTEMINCLUDE

        INCLUDEPATH += symbian \
                       ../../../symbian/qmfdataclient

        HEADERS += ../../../symbian/qmfdataclient/qmfdataclientservercommon.h \
                   ../../../symbian/qmfdataclient/qmfdatasession.h \
                   ../../../symbian/qmfdataclient/qmfdatastorage.h \
                   symbian/symbianfileinfo.h \
                   symbian/symbianfile.h \
                   symbian/symbiandir.h

        SOURCES += ../../../symbian/qmfdataclient/qmfdatasession.cpp \
                   ../../../symbian/qmfdataclient/qmfdatastorage.cpp \
                   symbian/symbianfileinfo.cpp \
                   symbian/symbianfile.cpp \
                   symbian/symbiandir.cpp

        LIBS += -lefsrv
    }

    load(data_caging_paths)

    TARGET.EPOCALLOWDLLDATA = 1
    TARGET.CAPABILITY = ALL -TCB
    TARGET.UID3 = 0x20034923

    deploy.path = C:
    pluginstub.sources = $${TARGET}.dll
    pluginstub.path = $$QT_PLUGINS_BASE_DIR/qtmail/contentmanagers
    DEPLOYMENT += pluginstub

    load(armcc_warnings)
}

include(../../../../common.pri)
