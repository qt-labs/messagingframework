TEMPLATE = lib 
CONFIG += warn_on
CONFIG += qmfclient

equals(QT_MAJOR_VERSION, 4){
    TARGET = qmfmessageserver
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfclient
    } else {
        LIBS += -lqmfclient
    }
}
equals(QT_MAJOR_VERSION, 5){
    TARGET = qmfmessageserver5
    macx:contains(QT_CONFIG, qt_framework) {
        LIBS += -framework qmfclient5
    } else {
        LIBS += -lqmfclient5
    }
}
target.path += $$QMF_INSTALL_ROOT/lib

QT = core network

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
    QT += gui
    equals(QT_MAJOR_VERSION, 5): QT += widgets
}

DEFINES += MESSAGESERVER_INTERNAL

DEPENDPATH += .

INCLUDEPATH += . ../qmfclient ../qmfclient/support

LIBS += -L../qmfclient/build
macx:LIBS += -F../qmfclient/build

PUBLIC_HEADERS += qmailauthenticator.h \
                  qmailmessagebuffer.h \
                  qmailmessageclassifier.h \
                  qmailmessageservice.h \
                  qmailserviceconfiguration.h \
                  qmailstoreaccountfilter.h \
                  qmailtransport.h \
                  qmailheartbeattimer.h

SOURCES += qmailauthenticator.cpp \
           qmailmessagebuffer.cpp \
           qmailmessageclassifier.cpp \
           qmailmessageservice.cpp \
           qmailserviceconfiguration.cpp \
           qmailstoreaccountfilter.cpp \
           qmailtransport.cpp \
           qmailheartbeattimer_qtimer.cpp # NB: There are multiple implementations

contains(DEFINES,MESSAGESERVER_PLUGINS) {
    PUBLIC_HEADERS += qmailmessageserverplugin.h
    SOURCES += qmailmessageserverplugin.cpp
}

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

equals(QT_MAJOR_VERSION, 4): header_files.path=$$QMF_INSTALL_ROOT/include/qmfmessageserver
equals(QT_MAJOR_VERSION, 5): header_files.path=$$QMF_INSTALL_ROOT/include/qmfmessageserver5
header_files.files=$$PUBLIC_HEADERS

INSTALLS += header_files

unix: {
	CONFIG += create_pc create_prl
	QMAKE_PKGCONFIG_LIBDIR  = $$target.path
	QMAKE_PKGCONFIG_INCDIR  = $$header_files.path
	QMAKE_PKGCONFIG_DESTDIR = pkgconfig
}
include(../../../common.pri)
