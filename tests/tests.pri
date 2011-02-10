macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET=10.5
}

target.path+=$$QMF_INSTALL_ROOT/tests

QMFPATH=../../src/libraries/qmfclient
DEPENDPATH += .
INCLUDEPATH += . $$QMFPATH $$QMFPATH/support
LIBS += -L$$QMFPATH/build
macx:LIBS += -F$$QMFPATH/build
!symbian:QMAKE_LFLAGS += -Wl,-rpath,$$QMFPATH

include(../common.pri)

