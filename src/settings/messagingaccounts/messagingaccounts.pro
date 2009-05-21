TEMPLATE = app
CONFIG -= debug_and_release

TARGET = messagingaccounts
target.path += $$QMF_INSTALL_ROOT/bin
INSTALLS += target

DEPENDPATH += .

INCLUDEPATH += . ../../libraries/qtopiamail \
../../libraries/qtopiamail/support \
../../libraries/messageserver

LIBS += -L../../libraries/qtopiamail -lqtopiamail \
-L../../libraries/messageserver -lmessageserver

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
