/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILSTORE_H
#define QMAILSTORE_H

#include "qmailmessage.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"
#include "qmailfolder.h"
#include "qmailfolderkey.h"
#include "qmailfoldersortkey.h"
#include "qmailaccount.h"
#include "qmailaccountkey.h"
#include "qmailaccountsortkey.h"
#include "qmailaccountconfiguration.h"
#include "qmailmessageremovalrecord.h"
#include "qmailglobal.h"

class QMailStore;
class QMailStorePrivate;

#ifdef QMAILSTOREINSTANCE_DEFINED_HERE
static QMailStore* QMailStoreInstance();
#endif

class QTOPIAMAIL_EXPORT QMailStore : public QObject
{
    Q_OBJECT

public:
    enum InitializationState
    {
        Uninitialized = 0,
        InitializationFailed,
        Initialized
    };

    enum ReturnOption
    {
        ReturnAll = 0,
        ReturnDistinct
    };

    enum MessageRemovalOption
    {
        NoRemovalRecord = 1,
        CreateRemovalRecord
    };

    enum ChangeType
    {
        Added = 1,
        Removed,
        Updated,
        ContentsModified
    };

    enum ErrorCode
    {
        NoError = 0,
        InvalidId, 
        ConstraintFailure,
        ContentInaccessible,
        NotYetImplemented,
        FrameworkFault
    };

public:
    virtual ~QMailStore();

    static InitializationState initializationState();

    QMailStore::ErrorCode lastError() const;

    bool addAccount(QMailAccount* account, QMailAccountConfiguration* config);
    bool addFolder(QMailFolder* f);
    bool addMessage(QMailMessage* m);
    bool addMessage(QMailMessageMetaData* m);

    bool removeAccount(const QMailAccountId& id);
    bool removeAccounts(const QMailAccountKey& key);

    bool removeFolder(const QMailFolderId& id, MessageRemovalOption option = NoRemovalRecord);
    bool removeFolders(const QMailFolderKey& key, MessageRemovalOption option = NoRemovalRecord);

    bool removeMessage(const QMailMessageId& id, MessageRemovalOption option = NoRemovalRecord);
    bool removeMessages(const QMailMessageKey& key, MessageRemovalOption option = NoRemovalRecord);

    bool updateAccount(QMailAccount* account, QMailAccountConfiguration* config = 0);
    bool updateAccountConfiguration(QMailAccountConfiguration* config);
    bool updateFolder(QMailFolder* f);
    bool updateMessage(QMailMessage* m);
    bool updateMessage(QMailMessageMetaData* m);
    bool updateMessagesMetaData(const QMailMessageKey& key, const QMailMessageKey::Properties& properties, const QMailMessageMetaData& data);
    bool updateMessagesMetaData(const QMailMessageKey& key, quint64 messageStatus, bool set);

    int countAccounts(const QMailAccountKey& key = QMailAccountKey()) const;
    int countFolders(const QMailFolderKey& key = QMailFolderKey()) const;
    int countMessages(const QMailMessageKey& key = QMailMessageKey()) const;

    int sizeOfMessages(const QMailMessageKey& key = QMailMessageKey()) const;

    const QMailAccountIdList queryAccounts(const QMailAccountKey& key = QMailAccountKey(), const QMailAccountSortKey& sortKey = QMailAccountSortKey()) const;
    const QMailFolderIdList queryFolders(const QMailFolderKey& key = QMailFolderKey(), const QMailFolderSortKey& sortKey = QMailFolderSortKey()) const;
    const QMailMessageIdList queryMessages(const QMailMessageKey& key = QMailMessageKey(), const QMailMessageSortKey& sortKey = QMailMessageSortKey()) const;

    QMailAccount account(const QMailAccountId& id) const;
    QMailAccountConfiguration accountConfiguration(const QMailAccountId& id) const;

    QMailFolder folder(const QMailFolderId& id) const;

    QMailMessage message(const QMailMessageId& id) const;
    QMailMessage message(const QString& uid, const QMailAccountId& accountId) const;

    QMailMessageMetaData messageMetaData(const QMailMessageId& id) const;
    QMailMessageMetaData messageMetaData(const QString& uid, const QMailAccountId& accountId) const;
    const QMailMessageMetaDataList messagesMetaData(const QMailMessageKey& key, const QMailMessageKey::Properties& properties, ReturnOption option = ReturnAll) const;

    const QMailMessageRemovalRecordList messageRemovalRecords(const QMailAccountId& parentAccountId, const QMailFolderId& parentFolderId = QMailFolderId()) const;

    bool purgeMessageRemovalRecords(const QMailAccountId& parentAccountId, const QStringList& serverUid = QStringList());

    bool restoreToPreviousFolder(const QMailMessageId& id);
    bool restoreToPreviousFolder(const QMailMessageKey& key);

    bool registerAccountStatusFlag(const QString& name);
    quint64 accountStatusMask(const QString& name) const;

    bool registerFolderStatusFlag(const QString& name);
    quint64 folderStatusMask(const QString& name) const;

    bool registerMessageStatusFlag(const QString& name);
    quint64 messageStatusMask(const QString& name) const;

    void setRetrievalInProgress(const QMailAccountIdList &ids);
    void setTransmissionInProgress(const QMailAccountIdList &ids);

    bool asynchronousEmission() const;

    void flushIpcNotifications();

    static QMailStore* instance();
#ifdef QMAILSTOREINSTANCE_DEFINED_HERE
    friend QMailStore* QMailStoreInstance();
#endif

signals:
    void errorOccurred(QMailStore::ErrorCode code);

    void accountsAdded(const QMailAccountIdList& ids);
    void accountsRemoved(const QMailAccountIdList& ids);
    void accountsUpdated(const QMailAccountIdList& ids);
    void accountContentsModified(const QMailAccountIdList& ids);

    void messagesAdded(const QMailMessageIdList& ids);
    void messagesRemoved(const QMailMessageIdList& ids);
    void messagesUpdated(const QMailMessageIdList& ids);
    void messageContentsModified(const QMailMessageIdList& ids);

    void foldersAdded(const QMailFolderIdList& ids);
    void foldersRemoved(const QMailFolderIdList& ids);
    void foldersUpdated(const QMailFolderIdList& ids);
    void folderContentsModified(const QMailFolderIdList& ids);

    void messageRemovalRecordsAdded(const QMailAccountIdList& ids);
    void messageRemovalRecordsRemoved(const QMailAccountIdList& ids);

    void retrievalInProgress(const QMailAccountIdList &ids);
    void transmissionInProgress(const QMailAccountIdList &ids);

private:
    friend class QMailStoreImplementationBase;
    friend class QMailStorePrivate;
    friend class tst_QMailStore;
    friend class tst_QMailStoreKeys;

    QMailStore();

    bool updateMessage(QMailMessageMetaData* m, QMailMessage* mail);

    void clearContent();

    void emitErrorNotification(QMailStore::ErrorCode code);
    void emitAccountNotification(ChangeType type, const QMailAccountIdList &ids);
    void emitFolderNotification(ChangeType type, const QMailFolderIdList &ids);
    void emitMessageNotification(ChangeType type, const QMailMessageIdList &ids);
    void emitRemovalRecordNotification(ChangeType type, const QMailAccountIdList &ids);
    void emitRetrievalInProgress(const QMailAccountIdList &ids);
    void emitTransmissionInProgress(const QMailAccountIdList &ids);

    QMailStorePrivate* d;
};

Q_DECLARE_USER_METATYPE_ENUM(QMailStore::MessageRemovalOption)

#endif
