QT -= gui
QT += testlib
CONFIG += unittest

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET=10.5
}

equals(QT_MAJOR_VERSION, 4){
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfclient
    } else {
        LIBS += -lqmfclient
    }
    target.path += $$QMF_INSTALL_ROOT/tests
}
equals(QT_MAJOR_VERSION, 5){
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfclient5
    } else {
        LIBS += -lqmfclient5
    }
    target.path += $$QMF_INSTALL_ROOT/tests5
}

QMFPATH=../../src/libraries/qmfclient
DEPENDPATH += .
INCLUDEPATH += . $$QMFPATH $$QMFPATH/support
LIBS += -L$$QMFPATH/build

macx:LIBS += -F$$QMFPATH/build
QMAKE_LFLAGS += -Wl,-rpath,$$QMFPATH

include(../common.pri)

