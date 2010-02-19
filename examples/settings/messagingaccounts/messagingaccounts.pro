TEMPLATE = app
TARGET = messagingaccounts
target.path += $$QMF_INSTALL_ROOT/bin
CONFIG += qtopiamail messageserver

DEPENDPATH += .

INCLUDEPATH += . ../../../src/libraries/qtopiamail \
                 ../../../src/libraries/qtopiamail/support \
                 ../../../src/libraries/messageserver

LIBS += -L../../../src/libraries/qtopiamail/build \
        -L../../../src/libraries/messageserver/build

HEADERS += accountsettings.h \
           editaccount.h \
           mmseditaccount.h \
           statusdisplay.h


SOURCES += accountsettings.cpp \
           editaccount.cpp \
           main.cpp \
           mmseditaccount.cpp \
           statusdisplay.cpp

RESOURCES += messagingaccounts.qrc

include(../../../common.pri)
