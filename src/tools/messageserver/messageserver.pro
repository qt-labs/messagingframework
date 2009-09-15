TEMPLATE = app

include(../../../common.pri)

TARGET = messageserver
target.path += $$QMF_INSTALL_ROOT/bin
INSTALLS += target

DEPENDPATH += . 

INCLUDEPATH += . ../../libraries/qtopiamail \
                 ../../libraries/qtopiamail/support \
                 ../../libraries/messageserver

LIBS += -L../../libraries/messageserver -lmessageserver \
        -L../../libraries/qtopiamail -lqtopiamail

HEADERS += mailmessageclient.h \
           messageserver.h \
           servicehandler.h \
           newcountnotifier.h

SOURCES += mailmessageclient.cpp \
           main.cpp \
           messageserver.cpp \
           prepareaccounts.cpp \
           newcountnotifier.cpp \
           servicehandler.cpp

TRANSLATIONS += messageserver-ar.ts \
                messageserver-de.ts \
                messageserver-en_GB.ts \
                messageserver-en_SU.ts \
                messageserver-en_US.ts \
                messageserver-es.ts \
                messageserver-fr.ts \
                messageserver-it.ts \
                messageserver-ja.ts \
                messageserver-ko.ts \
                messageserver-pt_BR.ts \
                messageserver-zh_CN.ts \
                messageserver-zh_TW.ts

