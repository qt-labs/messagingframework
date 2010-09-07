TEMPLATE = app
TARGET = messagingaccounts
target.path += $$QMF_INSTALL_ROOT/bin
CONFIG += qmf qmfmessageserver

DEPENDPATH += .

QTMAIL_EXAMPLE=../qtmail

INCLUDEPATH += . ../../src/libraries/qmf \
                 ../../src/libraries/qmf/support \
                 ../../src/libraries/qmfmessageserver \
                 $$QTMAIL_EXAMPLE/app \
                 $$QTMAIL_EXAMPLE/libs/qmfutil

LIBS += -L../../src/libraries/qmf/build \
        -L../../src/libraries/qmfmessageserver/build

macx:LIBS += -F../../src/libraries/qmf/build \
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
