TARGET     = QmfClient
QT         = core sql network
CONFIG    += warn_on

MODULE_PLUGIN_TYPES = \
    contentmanagers \
    crypto

load(qt_module)
CONFIG -= create_cmake

DEFINES -= QT_NO_CAST_TO_ASCII # this is a fight for another day.

DEFINES += QT_BUILD_QCOP_LIB QMF_INTERNAL
DEFINES += QMF_INSTALL_ROOT=\\\"$$QMF_INSTALL_ROOT\\\"

#DEPENDPATH += .
INCLUDEPATH += support

contains(DEFINES, USE_HTML_PARSER) {
    QT += gui
}


HEADERS += \
    qmailaccount.h \
    qmailaccountconfiguration.h \
    qmailaccountkey.h \
    qmailaccountlistmodel.h \
    qmailaccountsortkey.h \
    qmailaction.h \
    qmailaddress.h \
    qmailcodec.h \
    qmailcontentmanager.h \
    qmailcrypto.h \
    qmailcryptofwd.h \
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
    qmailthread.h \
    qmailthreadkey.h \
    qmailthreadlistmodel.h \
    qmailthreadsortkey.h \
    qprivateimplementation.h \
    qprivateimplementationdef_p.h \
    support/qmailglobal.h \
    support/qmaillog.h \
    support/qlogsystem.h \
    support/qloggers.h \
    support/qmailnamespace.h \
    support/qmailpluginmanager.h \
    support/qmailipc.h \
    bind_p.h \
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
    qmailthread_p.h \
    qmailthreadkey_p.h \
    qmailthreadsortkey_p.h \
    longstring_p.h \
    longstream_p.h \
    support/qcopchannel_p_p.h \
    support/qringbuffer_p.h \
    support/qcopadaptor_p.h \
    support/qcopapplicationchannel_p.h \
    support/qcopchannel_p.h \
    support/qcopchannelmonitor_p.h \
    support/qcopserver_p.h

SOURCES += longstream.cpp \
           longstring.cpp \
           locks.cpp \
           qmailaccount.cpp \
           qmailaccountconfiguration.cpp \
           qmailaccountkey.cpp \
           qmailaccountlistmodel.cpp \
           qmailaccountsortkey.cpp \
           qmailaction.cpp \
           qmailaddress.cpp \
           qmailcodec.cpp \
           qmailcontentmanager.cpp \
           qmailcrypto.cpp \
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
           qmailthread.cpp \
           qmailthreadkey.cpp \
           qmailthreadlistmodel.cpp \
           qmailthreadsortkey.cpp \
           qprivateimplementation.cpp \
           support/qmailnamespace.cpp \
           support/qmaillog.cpp \
           support/qlogsystem.cpp \
           support/qloggers.cpp \
           support/qcopadaptor.cpp \
           support/qcopapplicationchannel.cpp \
           support/qcopchannel.cpp \
           support/qcopchannelmonitor.cpp \
           support/qcopserver.cpp \
           support/qmailpluginmanager.cpp

RESOURCES += qmf.qrc \
             qmf_icons.qrc \
             qmf_qt.qrc

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

packagesExist(icu-uc) {
    LIBS_PRIVATE += -licui18n -licuuc -licudata
    PRIVATE_HEADERS += support/qcharsetdetector_p.h \
                       support/qcharsetdetector.h
    SOURCES += support/qcharsetdetector.cpp
    DEFINES += HAVE_LIBICU
} else {
    warning("icu not available, not doing character set detection")
}

