TARGET     = QmfClient
QT         = core dbus sql network core5compat
CONFIG    += warn_on

MODULE_PLUGIN_TYPES = \
    contentmanagers \
    crypto

load(qt_module)
CONFIG -= create_cmake

DEFINES += QMF_INTERNAL

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
    qmailstoreaccount.h \
    qmailtimestamp.h \
    qmailthread.h \
    qmailthreadkey.h \
    qmailthreadlistmodel.h \
    qmailthreadsortkey.h \
    qprivateimplementation.h \
    qprivateimplementationdef_p.h \
    support/qmailglobal.h \
    support/qmaillog.h \
    support/qmailnamespace.h \
    support/qmailpluginmanager.h \
    support/qmailipc.h \
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
    qmailstorenotifier_p.h \
    qmailstoreimplementation_p.h \
    qmailstoresql_p.h \
    qmailthread_p.h \
    qmailthreadkey_p.h \
    qmailthreadsortkey_p.h \
    longstring_p.h

SOURCES += \
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
           qmailfolderkey.cpp \
           qmailfoldersortkey.cpp \
           qmailid.cpp \
           qmailinstantiations.cpp \
           qmailkeyargument.cpp \
           qmailmessage.cpp \
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
           qmailstoreaccount.cpp \
           qmailstore_p.cpp \
           qmailstorenotifier_p.cpp \
           qmailstoreimplementation_p.cpp \
           qmailstoresql_p.cpp \
           qmailtimestamp.cpp \
           qmailthread.cpp \
           qmailthreadkey.cpp \
           qmailthreadlistmodel.cpp \
           qmailthreadsortkey.cpp \
           qprivateimplementation.cpp \
           support/qmailnamespace.cpp \
           support/qmaillog.cpp \
           support/qmailpluginmanager.cpp

mailservice.files = qmailservice.xml
mailservice.header_flags = -i qmailid.h -i qmailaction.h -i qmailserviceaction.h -i qmailstore.h
mailstore.files = qmailstore.xml
mailstore.header_flags = -i qmailid.h -i qmailstore.h
DBUS_INTERFACES += mailservice
DBUS_ADAPTORS += mailstore

RESOURCES += qmf.qrc \
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
    PRIVATE_HEADERS += support/qmailcharsetdetector_p.h \
                       support/qmailcharsetdetector.h
    SOURCES += support/qmailcharsetdetector.cpp
    DEFINES += HAVE_LIBICU
} else {
    warning("icu not available, not doing character set detection")
}

contains(DEFINES, USE_ACCOUNTS_QT) {
    packagesExist(accounts-qt5) {
        CONFIG += link_pkgconfig
        PKGCONFIG += accounts-qt5
        PRIVATE_HEADERS += libaccounts_p.h
        SOURCES += libaccounts_p.cpp
        DEFINES += "QMF_ACCOUNT_MANAGER_CLASS=LibAccountManager"

        provider.files = resources/email.provider
        provider.path  = $$QMF_INSTALL_ROOT/share/accounts/providers
                         service.files = resources/email.service
        service.path  = $$QMF_INSTALL_ROOT/share/accounts/services

        INSTALLS += provider service
    } else {
        warning("libaccounts not available, not using it as account manager.")
    }
}
