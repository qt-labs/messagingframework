CONFIG += qtestlib
TEMPLATE = app
TARGET = tst_messageserver 
target.path += $$QMF_INSTALL_ROOT/tests
INSTALLS += target
DEPENDPATH += . 3rdparty

IMAP_PLUGIN=../../../../plugins/messageservices/imap/
MESSAGE_SERVER = ../../

INCLUDEPATH += . 3rdparty ../../../../libraries/qtopiamail \
                 ../../../../libraries/qtopiamail/support \
                 ../../../../libraries/messageserver \
                 $$IMAP_PLUGIN \
                 $$MESSAGE_SERVER 

LIBS += -L../../../../libraries/messageserver -lmessageserver \
        -L../../../../libraries/qtopiamail -lqtopiamail

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



