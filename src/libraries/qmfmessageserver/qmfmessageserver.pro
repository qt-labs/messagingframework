TEMPLATE = lib 
CONFIG += warn_on
CONFIG += qmf
TARGET = qmfmessageserver

target.path += $$QMF_INSTALL_ROOT/lib

QT = core network
!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR):QT += gui

symbian: {
    MMP_RULES += EXPORTUNFROZEN
}

DEFINES += MESSAGESERVER_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../qmf ../qmf/support

LIBS += -L../qmf/build
macx:LIBS += -F../qmf/build

PUBLIC_HEADERS += qmailauthenticator.h \
                  qmailmessageclassifier.h \
                  qmailmessageservice.h \
                  qmailserviceconfiguration.h \
                  qmailstoreaccountfilter.h \
                  qmailtransport.h

HEADERS += $$PUBLIC_HEADERS

SOURCES += qmailauthenticator.cpp \
           qmailmessageclassifier.cpp \
           qmailmessageservice.cpp \
           qmailserviceconfiguration.cpp \
           qmailstoreaccountfilter.cpp \
           qmailtransport.cpp

header_files.path=$$QMF_INSTALL_ROOT/include/qmfmessageserver
header_files.files=$$PUBLIC_HEADERS

INSTALLS += header_files

include(../../../common.pri)
