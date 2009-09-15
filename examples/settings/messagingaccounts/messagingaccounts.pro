TEMPLATE = app

include(../../../common.pri)

TARGET = messagingaccounts
target.path += $$QMF_INSTALL_ROOT/bin
INSTALLS += target

DEPENDPATH += .

INCLUDEPATH += . ../../../src/libraries/qtopiamail \
                 ../../../src/libraries/qtopiamail/support \
                 ../../../src/libraries/messageserver

LIBS += -L../../../src/libraries/qtopiamail -lqtopiamail \
        -L../../../src/libraries/messageserver -lmessageserver

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

