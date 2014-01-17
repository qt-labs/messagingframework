TEMPLATE = app
target.path += $$QMF_INSTALL_ROOT/bin
CONFIG += qmfclient qmfmessageserver

equals(QT_MAJOR_VERSION, 4){
    TARGET = messagingaccounts

    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfmessageserver -framework qmfclient
    } else {
        LIBS += -lqmfmessageserver -lqmfclient
    }
}
equals(QT_MAJOR_VERSION, 5){
    TARGET = messagingaccounts5
    QT += widgets

    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfmessageserver5 -framework qmfclient5
    } else {
        LIBS += -lqmfmessageserver5 -lqmfclient5
    }
}

DEPENDPATH += .

QTMAIL_EXAMPLE=../qtmail

#Required to build on windows
DEFINES += QMFUTIL_INTERNAL

INCLUDEPATH += . ../../src/libraries/qmfclient \
                 ../../src/libraries/qmfclient/support \
                 ../../src/libraries/qmfmessageserver \
                 $$QTMAIL_EXAMPLE/app \
                 $$QTMAIL_EXAMPLE/libs/qmfutil

LIBS += -L../../src/libraries/qmfclient/build \
        -L../../src/libraries/qmfmessageserver/build

macx:LIBS += -F../../src/libraries/qmfclient/build \
        -F../../src/libraries/qmfmessageserver/build

HEADERS += $$QTMAIL_EXAMPLE/app/accountsettings.h \
           $$QTMAIL_EXAMPLE/app/editaccount.h \
           $$QTMAIL_EXAMPLE/app/statusbar.h \
           $$QTMAIL_EXAMPLE/libs/qmfutil/qtmailnamespace.h

SOURCES += $$QTMAIL_EXAMPLE/app/accountsettings.cpp \
           $$QTMAIL_EXAMPLE/app/editaccount.cpp \
           main_messagingaccounts.cpp \
           $$QTMAIL_EXAMPLE/app/statusbar.cpp \
           $$QTMAIL_EXAMPLE/libs/qmfutil/qtmailnamespace.cpp

RESOURCES += messagingaccounts.qrc

include(../../common.pri)
