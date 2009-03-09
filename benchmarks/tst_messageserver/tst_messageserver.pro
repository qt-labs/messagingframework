CONFIG += qtestlib unittest
TEMPLATE = app
TARGET = tst_messageserver 
target.path += $$QMF_INSTALL_ROOT/tests
INSTALLS += target
DEPENDPATH += . 3rdparty

BASE=../../

IMAP_PLUGIN=$$BASE/src/plugins/messageservices/imap/
MESSAGE_SERVER=$$BASE/src/tools/messageserver

INCLUDEPATH += . 3rdparty $$BASE/src/libraries/qtopiamail \
                 $$BASE/src/libraries/qtopiamail/support \
                 $$BASE/src/libraries/messageserver \
                 $$IMAP_PLUGIN \
                 $$MESSAGE_SERVER 

LIBS += -L$$BASE/src/libraries/messageserver -lmessageserver \
        -L$$BASE/src/libraries/qtopiamail -lqtopiamail

QMAKE_LFLAGS += -Wl,-rpath,$$BASE/src/libraries/qtopiamail \
    -Wl,-rpath,$$BASE/src/libraries/messageserver

HEADERS += benchmarkcontext.h \
           qscopedconnection.h \
           testfsusage.h \
           testmalloc.h \
           3rdparty/cycle_p.h \
           $$IMAP_PLUGIN/imapconfiguration.h \
           $$MESSAGE_SERVER/mailmessageclient.h \
           $$MESSAGE_SERVER/messageserver.h \
           $$MESSAGE_SERVER/servicehandler.h \
           $$MESSAGE_SERVER/newcountnotifier.h

SOURCES += benchmarkcontext.cpp \
           qscopedconnection.cpp \
           testfsusage.cpp \
           testmalloc.cpp \
           tst_messageserver.cpp \
           $$IMAP_PLUGIN/imapconfiguration.cpp \
           $$MESSAGE_SERVER/mailmessageclient.cpp \
           $$MESSAGE_SERVER/messageserver.cpp \
           $$MESSAGE_SERVER/prepareaccounts.cpp \
           $$MESSAGE_SERVER/servicehandler.cpp \
           $$MESSAGE_SERVER/newcountnotifier.cpp



