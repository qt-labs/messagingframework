TEMPLATE = app
TARGET = messagingaccounts
target.path += $$QMF_INSTALL_ROOT/bin
CONFIG += qtopiamail messageserver

DEPENDPATH += .

QTMAIL_EXAMPLE=../../qtmail

INCLUDEPATH += . ../../../src/libraries/qtopiamail \
                 ../../../src/libraries/qtopiamail/support \
                 ../../../src/libraries/messageserver \
                 $$QTMAIL_EXAMPLE/app/settings \
                 $$QTMAIL_EXAMPLE/app \
                 $$QTMAIL_EXAMPLE/libs/qmfutil

LIBS += -L../../../src/libraries/qtopiamail/build \
        -L../../../src/libraries/messageserver/build

macx:LIBS += -F../../../src/libraries/qtopiamail/build \
        -F../../../src/libraries/messageserver/build

HEADERS += $$QTMAIL_EXAMPLE/app/settings/accountsettings.h \
           $$QTMAIL_EXAMPLE/app/settings/editaccount.h \
           $$QTMAIL_EXAMPLE/app/mmseditaccount.h \
           $$QTMAIL_EXAMPLE/app/statusbar.h \
           $$QTMAIL_EXAMPLE/libs/qmfutil/qtmailnamespace.h

SOURCES += $$QTMAIL_EXAMPLE/app/settings/accountsettings.cpp \
           $$QTMAIL_EXAMPLE/app/settings/editaccount.cpp \
           main_messagingaccounts.cpp \
           $$QTMAIL_EXAMPLE/app/mmseditaccount.cpp \
           $$QTMAIL_EXAMPLE/app/statusbar.cpp \
           $$QTMAIL_EXAMPLE/libs/qmfutil/qtmailnamespace.cpp

RESOURCES += messagingaccounts.qrc

include(../../../common.pri)
