/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILSTOREIMPLEMENTATION_P_H
#define QMAILSTOREIMPLEMENTATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmailstore.h"

#include <QList>
#include <QPair>
#include <QString>
#include <QTimer>


class QMailStoreImplementationBase : public QObject
{
    Q_OBJECT

public:
    QMailStoreImplementationBase(QMailStore* parent);

    virtual bool initStore();
    static bool initialized();

    QMailStore::ErrorCode lastError() const;
    void setLastError(QMailStore::ErrorCode code) const;

    bool asynchronousEmission() const;

    void flushIpcNotifications();

    void notifyAccountsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids);
    void notifyMessagesChange(QMailStore::ChangeType changeType, const QMailMessageIdList& ids);
    void notifyFoldersChange(QMailStore::ChangeType changeType, const QMailFolderIdList& ids);
    void notifyMessageRemovalRecordsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids);

    static QString accountAddedSig();
    static QString accountRemovedSig();
    static QString accountUpdatedSig();
    static QString accountContentsModifiedSig();

    static QString messageAddedSig();
    static QString messageRemovedSig();
    static QString messageUpdatedSig();
    static QString messageContentsModifiedSig();

    static QString folderAddedSig();
    static QString folderUpdatedSig();
    static QString folderRemovedSig();
    static QString folderContentsModifiedSig();

    static QString messageRemovalRecordsAddedSig();
    static QString messageRemovalRecordsRemovedSig();

    static const int maxNotifySegmentSize = 0;

public slots:
    void processIpcMessageQueue();
    void ipcMessage(const QString& message, const QByteArray& data);
    void flushNotifications();
    void aboutToQuit();

protected:
    typedef void (QMailStore::*AccountUpdateSignal)(const QMailAccountIdList&);
    typedef QMap<QString, AccountUpdateSignal> AccountUpdateSignalMap;
    static AccountUpdateSignalMap initAccountUpdateSignals();

    typedef void (QMailStore::*FolderUpdateSignal)(const QMailFolderIdList&);
    typedef QMap<QString, FolderUpdateSignal> FolderUpdateSignalMap;
    static FolderUpdateSignalMap initFolderUpdateSignals();

    typedef void (QMailStore::*MessageUpdateSignal)(const QMailMessageIdList&);
    typedef QMap<QString, MessageUpdateSignal> MessageUpdateSignalMap;
    static MessageUpdateSignalMap initMessageUpdateSignals();

    static bool init;
    
    virtual void emitIpcNotification(AccountUpdateSignal signal, const QMailAccountIdList &ids);
    virtual void emitIpcNotification(FolderUpdateSignal signal, const QMailFolderIdList &ids);
    virtual void emitIpcNotification(MessageUpdateSignal signal, const QMailMessageIdList &ids);

private:
    bool emitIpcNotification();

    QMailStore* q;
    
    mutable QMailStore::ErrorCode errorCode;

    bool asyncEmission;

    QBasicTimer preFlushTimer;
    QTimer flushTimer;

    QSet<QMailAccountId> addAccountsBuffer;
    QSet<QMailFolderId> addFoldersBuffer;
    QSet<QMailMessageId> addMessagesBuffer;
    QSet<QMailAccountId> addMessageRemovalRecordsBuffer;

    QSet<QMailMessageId> updateMessagesBuffer;
    QSet<QMailFolderId> updateFoldersBuffer;
    QSet<QMailAccountId> updateAccountsBuffer;

    QSet<QMailAccountId> removeMessageRemovalRecordsBuffer;
    QSet<QMailMessageId> removeMessagesBuffer;
    QSet<QMailFolderId> removeFoldersBuffer;
    QSet<QMailAccountId> removeAccountsBuffer;

    QSet<QMailFolderId> folderContentsModifiedBuffer;
    QSet<QMailAccountId> accountContentsModifiedBuffer;
    QSet<QMailMessageId> messageContentsModifiedBuffer;

    QTimer queueTimer;
    QList<QPair<QString, QByteArray> > messageQueue;
};


class QMailStoreImplementation : public QMailStoreImplementationBase
{
public:
    QMailStoreImplementation(QMailStore* parent);

    virtual bool initStore() = 0;
    virtual void clearContent() = 0;

    virtual bool addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                            QMailAccountIdList *addedAccountIds) = 0;

    virtual bool addFolder(QMailFolder *f,
                           QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool addMessage(QMailMessage *m,
                            QMailMessageIdList *addedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool addMessage(QMailMessageMetaData *m,
                            QMailMessageIdList *addedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool removeAccounts(const QMailAccountKey &key,
                                QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages) = 0;

    virtual bool removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                               QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailAccountIdList *modifiedAccounts) = 0;

    virtual bool removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                                QMailMessageIdList *deletedMessages, QMailAccountIdList *modifiedAccounts, QMailFolderIdList *modifiedFolders) = 0;

    virtual bool updateAccount(QMailAccount *account, QMailAccountConfiguration* config,
                               QMailAccountIdList *updatedAccountIds) = 0;

    virtual bool updateAccountConfiguration(QMailAccountConfiguration* config,
                                            QMailAccountIdList *updatedAccountIds) = 0;

    virtual bool updateFolder(QMailFolder* f,
                              QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateMessage(QMailMessageMetaData *metaData, QMailMessage *mail,
                               QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, bool *modifiedContent) = 0;

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data,
                                        QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, quint64 messageStatus, bool set,
                                        QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool restoreToPreviousFolder(const QMailMessageKey &key,
                                         QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool purgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids) = 0;

    virtual int countAccounts(const QMailAccountKey &key) const = 0;
    virtual int countFolders(const QMailFolderKey &key) const = 0;
    virtual int countMessages(const QMailMessageKey &key) const = 0;

    virtual int sizeOfMessages(const QMailMessageKey &key) const = 0;

    virtual QMailAccountIdList queryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey) const = 0;
    virtual QMailFolderIdList queryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey) const = 0;
    virtual QMailMessageIdList queryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey) const = 0;

    virtual QMailAccount account(const QMailAccountId &id) const = 0;
    virtual QMailAccountConfiguration accountConfiguration(const QMailAccountId &id) const = 0;

    virtual QMailFolder folder(const QMailFolderId &id) const = 0;

    virtual QMailMessage message(const QMailMessageId &id) const = 0;
    virtual QMailMessage message(const QString &uid, const QMailAccountId &accountId) const = 0;

    virtual QMailMessageMetaData messageMetaData(const QMailMessageId &id) const = 0;
    virtual QMailMessageMetaData messageMetaData(const QString &uid, const QMailAccountId &accountId) const = 0;
    virtual QMailMessageMetaDataList messagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option) const = 0;

    virtual QMailMessageRemovalRecordList messageRemovalRecords(const QMailAccountId &parentAccountId, const QMailFolderId &parentFolderId) const = 0;

    virtual bool registerAccountStatusFlag(const QString &name) = 0;
    virtual quint64 accountStatusMask(const QString &name) const = 0;

    virtual bool registerFolderStatusFlag(const QString &name) = 0;
    virtual quint64 folderStatusMask(const QString &name) const = 0;

    virtual bool registerMessageStatusFlag(const QString &name) = 0;
    virtual quint64 messageStatusMask(const QString &name) const = 0;
};

#endif

