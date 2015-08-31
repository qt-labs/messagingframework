QT -= gui
QT += testlib
CONFIG += testcase

QT += qmfclient qmfclient-private
target.path += $$QMF_INSTALL_ROOT/tests5

include(../common.pri)

