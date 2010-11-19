TEMPLATE = app
TARGET = messagingaccounts
target.path += $$QMF_INSTALL_ROOT/bin
CONFIG += qmfclient qmfmessageserver

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
