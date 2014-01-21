QT -= gui
QT += testlib
CONFIG += unittest

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET=10.5
}

QT += qmfclient qmfclient-private
target.path += $$QMF_INSTALL_ROOT/tests5

include(../common.pri)

