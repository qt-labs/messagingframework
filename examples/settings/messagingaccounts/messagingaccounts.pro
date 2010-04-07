TEMPLATE = app
TARGET = messagingaccounts
target.path += $$QMF_INSTALL_ROOT/bin
CONFIG += qtopiamail messageserver qmfutil

DEPENDPATH += .

QTMAIL_EXAMPLE=../../applications/qtmail

INCLUDEPATH += . ../../../src/libraries/qtopiamail \
                 ../../../src/libraries/qtopiamail/support \
                 ../../../src/libraries/messageserver \
                 ../../../src/libraries/qmfutil \
                 $$QTMAIL_EXAMPLE/settings \
                 $$QTMAIL_EXAMPLE

LIBS += -L../../../src/libraries/qtopiamail/build \
        -L../../../src/libraries/messageserver/build \
        -L../../../src/libraries/qmfutil/build \

macx:LIBS += -F../../../src/libraries/qtopiamail/build \
        -F../../../src/libraries/messageserver/build \
        -F../../../src/libraries/qmfutil/build


HEADERS += $$QTMAIL_EXAMPLE/settings/accountsettings.h \
           $$QTMAIL_EXAMPLE/settings/editaccount.h \
           $$QTMAIL_EXAMPLE/mmseditaccount.h \
           $$QTMAIL_EXAMPLE/statusbar.h


SOURCES += $$QTMAIL_EXAMPLE/settings/accountsettings.cpp \
           $$QTMAIL_EXAMPLE/settings/editaccount.cpp \
           main_messagingaccounts.cpp \
           $$QTMAIL_EXAMPLE/mmseditaccount.cpp \
           $$QTMAIL_EXAMPLE/statusbar.cpp

RESOURCES += messagingaccounts.qrc

include(../../../common.pri)
