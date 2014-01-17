TEMPLATE = app

equals(QT_MAJOR_VERSION, 4){
    TARGET = serverobserver
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfmessageserver -framework qmfclient
    } else {
        LIBS += -lqmfmessageserver -lqmfclient
    }
}
equals(QT_MAJOR_VERSION, 5){
    TARGET = serverobserver5
    QT += widgets
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfmessageserver5 -framework qmfclient5
    } else {
        LIBS += -lqmfmessageserver5 -lqmfclient5
    }
}

target.path += $$QMF_INSTALL_ROOT/bin
CONFIG += qmfclient qmfmessageserver

DEPENDPATH += .
INCLUDEPATH += . ../../src/libraries/qmfclient \
                 ../../src/libraries/qmfclient/support \
                 ../../src/libraries/qmfmessageserver \

LIBS += -L../../src/libraries/qmfclient/build \
        -L../../src/libraries/qmfmessageserver/build

macx:LIBS += -F../../src/libraries/qmfclient/build \
        -F../../src/libraries/qmfmessageserver/build

HEADERS += serverobserver.h

SOURCES += serverobserver.cpp \
	   main.cpp

include(../../common.pri)
