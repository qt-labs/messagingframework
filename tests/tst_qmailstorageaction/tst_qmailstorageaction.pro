TEMPLATE = app
CONFIG += qmfclient
TARGET = tst_qmailstorageaction

SOURCES += tst_qmailstorageaction.cpp

include(../tests.pri)

# until we figure out how to get messageserver running inside the CI
# environment...
CONFIG += insignificant_test
