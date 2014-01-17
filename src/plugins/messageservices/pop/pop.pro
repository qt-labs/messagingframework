TEMPLATE = lib
TARGET = pop
CONFIG += qmfclient qmfmessageserver plugin

equals(QT_MAJOR_VERSION, 4) {
    target.path = $$QMF_INSTALL_ROOT/lib/qmf/plugins/messageservices
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfmessageserver -framework qmfclient
    } else {
        LIBS += -lqmfmessageserver -lqmfclient
    }
}
equals(QT_MAJOR_VERSION, 5) {
    target.path = $$QMF_INSTALL_ROOT/lib/qmf/plugins5/messageservices
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfmessageserver5 -framework qmfclient5
    } else {
        LIBS += -lqmfmessageserver5 -lqmfclient5
    }
}

QT = core network

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmfclient \
               ../../../libraries/qmfmessageserver \
               ../../../libraries/qmfclient/support

LIBS += -L../../../libraries/qmfclient/build \
        -L../../../libraries/qmfmessageserver/build
macx:LIBS += -F../../../libraries/qmfclient/build \
        -F../../../libraries/qmfmessageserver/build


HEADERS += popclient.h \
           popconfiguration.h \
           popservice.h \
           popauthenticator.h

SOURCES += popclient.cpp \
           popconfiguration.cpp \
           popservice.cpp \
           popauthenticator.cpp

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
    QT += gui
    equals(QT_MAJOR_VERSION, 5): QT += widgets

HEADERS += \
           popsettings.h

FORMS += popsettings.ui

SOURCES += \
           popsettings.cpp \
}

include(../../../../common.pri)
