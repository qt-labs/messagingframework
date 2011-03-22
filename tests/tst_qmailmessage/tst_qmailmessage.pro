TEMPLATE = app
CONFIG += qtestlib unittest qmfclient
TARGET = tst_qmailmessage

SOURCES += tst_qmailmessage.cpp

symbian: {
    addFiles.sources = symbiantestdata/*
    addFiles.path = symbiantestdata
    DEPLOYMENT += addFiles
}

include(../tests.pri)
