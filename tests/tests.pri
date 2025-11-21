QT -= gui
QT += testlib
CONFIG += testcase

QT += qmfclient qmfclient-private
target.path += $$QMF_INSTALL_ROOT/tests5

INSTALLS += target

# hack _p.h to work without the private/ on include
INCLUDEPATH += $$PWD/../src/libraries/qmfclient/
INCLUDEPATH += $$PWD/../src/libraries/qmfmessageserver/
