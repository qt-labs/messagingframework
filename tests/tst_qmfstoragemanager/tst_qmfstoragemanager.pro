TEMPLATE = app
TARGET = tst_qmfstoragemanager
CONFIG += unitest qmfclient qtestlib

QMF_STORAGEMANAGER_PATH = ../../src/plugins/contentmanagers/qmfstoragemanager

SOURCES += tst_qmfstoragemanager.cpp
INCLUDEPATH += $$QMF_STORAGEMANAGER_PATH

LIBS += -L$$QMF_STORAGEMANAGER_PATH/build
macx:LIBS += -F$$QMF_STORAGEMANAGER_PATH/build

qtAddLibrary(qmfstoragemanager)

include(../tests.pri)
