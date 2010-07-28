TEMPLATE = lib 
CONFIG += warn_on
TARGET = qtopiamail

target.path += $$QMF_INSTALL_ROOT/lib 

DEFINES += QT_BUILD_QCOP_LIB QTOPIAMAIL_INTERNAL
win32: {
    # QLocalSocket is broken on win32 prior to 4.5.2
    lessThan(QT_MAJOR_VERSION,5):lessThan(QT_MINOR_VERSION,6):lessThan(QT_PATCH_VERSION,2):DEFINES += QT_NO_QCOP_LOCAL_SOCKET
}

QT *= sql network
symbian: {
    LIBS += -lefsrv
    MMP_RULES += EXPORTUNFROZEN
}

DEPENDPATH += .

INCLUDEPATH += support

PUBLIC_HEADERS += qmailaccount.h \
                  qmailaccountconfiguration.h \
                  qmailaccountkey.h \
                  qmailaccountlistmodel.h \
                  qmailaccountsortkey.h \
                  qmailaction.h \
                  qmailaddress.h \
                  qmailcodec.h \
                  qmailcontentmanager.h \
                  qmaildatacomparator.h \
                  qmaildisconnected.h \
                  qmailfolder.h \
                  qmailfolderfwd.h \
                  qmailfolderkey.h \
                  qmailfoldersortkey.h \
                  qmailid.h \
                  qmailkeyargument.h \
                  qmailmessage.h \
                  qmailmessagefwd.h \
                  qmailmessagekey.h \
                  qmailmessagelistmodel.h \
                  qmailmessagemodelbase.h \
                  qmailmessageremovalrecord.h \
                  qmailmessageserver.h \
                  qmailmessageset.h \
                  qmailmessagesortkey.h \
                  qmailmessagethreadedmodel.h \
                  qmailserviceaction.h \
                  qmailsortkeyargument.h \
                  qmailstore.h \
                  qmailtimestamp.h \
                  qprivateimplementation.h \
                  qprivateimplementationdef.h \
                  support/qmailglobal.h \
                  support/qmaillog.h \
                  support/qmailnamespace.h \
                  support/qmailpluginmanager.h \
                  support/qmailipc.h

PRIVATE_HEADERS += bind_p.h \
                   locks_p.h \
                   mailkeyimpl_p.h \
                   mailsortkeyimpl_p.h \
                   qmailaccountkey_p.h \
                   qmailaccountsortkey_p.h \
                   qmailfolderkey_p.h \
                   qmailfoldersortkey_p.h \
                   qmailmessage_p.h \
                   qmailmessagekey_p.h \
                   qmailmessageset_p.h \
                   qmailmessagesortkey_p.h \
                   qmailserviceaction_p.h \
                   qmailstore_p.h \
                   qmailstoreimplementation_p.h \
                   longstring_p.h \
                   longstream_p.h \
                   support/qcopchannel_p.h \
                   support/qringbuffer_p.h \
                   support/qcopadaptor.h \
                   support/qcopapplicationchannel.h \
                   support/qcopchannel.h \
                   support/qcopchannelmonitor.h \
                   support/qcopserver.h

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

SOURCES += longstream.cpp \
           longstring.cpp \
           qmailaccount.cpp \
           qmailaccountconfiguration.cpp \
           qmailaccountkey.cpp \
           qmailaccountlistmodel.cpp \
           qmailaccountsortkey.cpp \
           qmailaction.cpp \
           qmailaddress.cpp \
           qmailcodec.cpp \
           qmailcontentmanager.cpp \
           qmaildatacomparator.cpp \
           qmaildisconnected.cpp \
           qmailfolder.cpp \
           qmailfolderfwd.cpp \
           qmailfolderkey.cpp \
           qmailfoldersortkey.cpp \
           qmailid.cpp \
           qmailinstantiations.cpp \
           qmailkeyargument.cpp \
           qmailmessage.cpp \
           qmailmessagefwd.cpp \
           qmailmessagekey.cpp \
           qmailmessagelistmodel.cpp \
           qmailmessagemodelbase.cpp \
           qmailmessageremovalrecord.cpp \
           qmailmessageserver.cpp \
           qmailmessageset.cpp \
           qmailmessagesortkey.cpp \
           qmailmessagethreadedmodel.cpp \
           qmailserviceaction.cpp \
           qmailstore.cpp \
           qmailstore_p.cpp \
           qmailstoreimplementation_p.cpp \
           qmailtimestamp.cpp \
           qprivateimplementation.cpp \
           support/qmailnamespace.cpp \
           support/qmaillog.cpp \
           support/qcopadaptor.cpp \
           support/qcopapplicationchannel.cpp \
           support/qcopchannel.cpp \
           support/qcopchannelmonitor.cpp \
           support/qcopserver.cpp \
           support/qmailpluginmanager.cpp

win32: {
    SOURCES += locks_win32.cpp
} else {
    SOURCES += locks.cpp
}

RESOURCES += qtopiamail.qrc \
             qtopiamail_icons.qrc \
             qtopiamail_qt.qrc

TRANSLATIONS += libqtopiamail-ar.ts \
                libqtopiamail-de.ts \
                libqtopiamail-en_GB.ts \
                libqtopiamail-en_SU.ts \
                libqtopiamail-en_US.ts \
                libqtopiamail-es.ts \
                libqtopiamail-fr.ts \
                libqtopiamail-it.ts \
                libqtopiamail-ja.ts \
                libqtopiamail-ko.ts \
                libqtopiamail-pt_BR.ts \
                libqtopiamail-zh_CN.ts \
                libqtopiamail-zh_TW.ts

header_files.path=$$QMF_INSTALL_ROOT/include/qtopiamail
header_files.files=$$PUBLIC_HEADERS

INSTALLS += header_files 

include(../../../common.pri)
