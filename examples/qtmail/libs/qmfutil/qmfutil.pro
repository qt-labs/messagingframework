TEMPLATE = lib 
CONFIG += warn_on
CONFIG += qmfclient
TARGET = qmfutil 

target.path += $$QMF_INSTALL_ROOT/lib

DEFINES += QMFUTIL_INTERNAL

symbian: {
    TARGET.EPOCALLOWDLLDATA = 1
    TARGET.CAPABILITY = ALL -TCB
    MMP_RULES += EXPORTUNFROZEN

    QMFUtil.sources = $${TARGET}.dll
    QMFUtil.path = /sys/bin
    DEPLOYMENT += QMFUtil
}

DEPENDPATH += .

INCLUDEPATH += . ../../../../src/libraries/qmfclient ../../../../src/libraries/qmfclient/support

macx:LIBS += -F../../../../src/libraries/qmfclient/build
LIBS += -L../../../../src/libraries/qmfclient/build

HEADERS += emailfoldermodel.h \
           emailfolderview.h \
           folderdelegate.h \
           foldermodel.h \
           folderview.h \
           qmailcomposer.h \
           qmailviewer.h \
           selectfolder.h \
           qtmailnamespace.h

SOURCES += emailfoldermodel.cpp \
           emailfolderview.cpp \
           folderdelegate.cpp \
           foldermodel.cpp \
           folderview.cpp \
           qmailcomposer.cpp \
           qmailviewer.cpp \
           selectfolder.cpp \
           qtmailnamespace.cpp

TRANSLATIONS += libqmfutil-ar.ts \
                libqmfutil-de.ts \
                libqmfutil-en_GB.ts \
                libqmfutil-en_SU.ts \
                libqmfutil-en_US.ts \
                libqmfutil-es.ts \
                libqmfutil-fr.ts \
                libqmfutil-it.ts \
                libqmfutil-ja.ts \
                libqmfutil-ko.ts \
                libqmfutil-pt_BR.ts \
                libqmfutil-zh_CN.ts \
                libqmfutil-zh_TW.ts

RESOURCES += qmfutil.qrc

include(../../../../common.pri)

