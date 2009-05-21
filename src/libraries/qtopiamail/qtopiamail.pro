TEMPLATE = lib 

TARGET = qtopiamail
target.path += $$QMF_INSTALL_ROOT/lib 
INSTALLS += target

DEFINES += QT_BUILD_QCOP_LIB QTOPIAMAIL_INTERNAL

QT *= sql network
symbian: {
    LIBS += -lefsrv
    MMP_RULES += EXPORTUNFROZEN
}

CONFIG += warn_on

DEPENDPATH += .

INCLUDEPATH += support

HEADERS += bind_p.h \
           locks_p.h \
           longstream_p.h \
           longstring_p.h \
           mailkeyimpl_p.h \
           qmailaccount.h \
           qmailaccountconfiguration.h \
           qmailaccountkey.h \
           qmailaccountkey_p.h \
           qmailaccountlistmodel.h \
           qmailaccountsortkey.h \
           qmailaccountsortkey_p.h \
           qmailaddress.h \
           qmailcodec.h \
           qmailcontentmanager.h \
           qmaildatacomparator.h \
           qmailfolder.h \
           qmailfolderkey.h \
           qmailfolderkey_p.h \
           qmailfoldersortkey.h \
           qmailfoldersortkey_p.h \
           qmailid.h \
           qmailkeyargument.h \
           qmailmessage.h \
           qmailmessagefwd.h \
           qmailmessagekey.h \
           qmailmessagekey_p.h \
           qmailmessagelistmodel.h \
           qmailmessagemodelbase.h \
           qmailmessageremovalrecord.h \
           qmailmessageserver.h \
           qmailmessageset.h \
           qmailmessagesortkey.h \
           qmailmessagesortkey_p.h \
           qmailmessagethreadedmodel.h \
           qmailserviceaction.h \
           qmailstore.h \
           qmailstore_p.h \
           qmailstoreimplementation_p.h \
           qmailtimestamp.h \
           qprivateimplementation.h \
           qprivateimplementationdef.h \
           support/qmailglobal.h \
           support/qmaillog.h \
           support/qmailnamespace.h \
           support/qcopadaptor.h \
           support/qcopapplicationchannel.h \
           support/qcopchannel.h \
           support/qcopchannel_p.h \
           support/qcopchannelmonitor.h \
           support/qcopserver.h \
           support/qmailpluginmanager.h \
           support/qringbuffer_p.h

SOURCES += locks.cpp \
           longstream.cpp \
           longstring.cpp \
           qmailaccount.cpp \
           qmailaccountconfiguration.cpp \
           qmailaccountkey.cpp \
           qmailaccountlistmodel.cpp \
           qmailaccountsortkey.cpp \
           qmailaddress.cpp \
           qmailcodec.cpp \
           qmailcontentmanager.cpp \
           qmaildatacomparator.cpp \
           qmailfolder.cpp \
           qmailfolderkey.cpp \
           qmailfoldersortkey.cpp \
           qmailid.cpp \
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

RESOURCES += qtopiamail.qrc \
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
