/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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

QT_BEGIN_NAMESPACE

class QFileSystemWatcher;

QT_END_NAMESPACE

class QMF_EXPORT QMailStoreImplementationBase : public QObject
{
    Q_OBJECT

public:
    QMailStoreImplementationBase(QMailStore* parent);
    virtual ~QMailStoreImplementationBase();

    void initialize();
    static QMailStore::InitializationState initializationState();

    QMailStore::ErrorCode lastError() const;
    void setLastError(QMailStore::ErrorCode code) const;

    bool asynchronousEmission() const;

    void flushIpcNotifications();

    void notifyAccountsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids);
    void notifyMessagesChange(QMailStore::ChangeType changeType, const QMailMessageIdList& ids);
    void notifyMessagesDataChange(QMailStore::ChangeType changeType, const QMailMessageMetaDataList& data);
    void notifyMessagesDataChange(const QMailMessageIdList& ids,  const QMailMessageKey::Properties& properties,
                                                                const QMailMessageMetaData& data);
    void notifyMessagesDataChange(const QMailMessageIdList& ids, quint64 status, bool set);
    void notifyFoldersChange(QMailStore::ChangeType changeType, const QMailFolderIdList& ids);
    void notifyThreadsChange(QMailStore::ChangeType changeType, const QMailThreadIdList &ids);
    void notifyMessageRemovalRecordsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids);
    void notifyRetrievalInProgress(const QMailAccountIdList& ids);
    void notifyTransmissionInProgress(const QMailAccountIdList& ids);

    bool setRetrievalInProgress(const QMailAccountIdList &ids);
    bool setTransmissionInProgress(const QMailAccountIdList &ids);

    bool isIpcConnectionEstablished() const;

    void disconnectIpc();
    void reconnectIpc();

    static QString accountAddedSig();
    static QString accountRemovedSig();
    static QString accountUpdatedSig();
    static QString accountContentsModifiedSig();

    static QString messageAddedSig();
    static QString messageRemovedSig();
    static QString messageUpdatedSig();
    static QString messageContentsModifiedSig();

    static QString messageMetaDataAddedSig();
    static QString messageMetaDataUpdatedSig();
    static QString messagePropertyUpdatedSig();
    static QString messageStatusUpdatedSig();

    static QString folderAddedSig();
    static QString folderUpdatedSig();
    static QString folderRemovedSig();
    static QString folderContentsModifiedSig();

    static QString threadAddedSig();
    static QString threadUpdatedSig();
    static QString threadRemovedSig();
    static QString threadContentsModifiedSig();

    static QString messageRemovalRecordsAddedSig();
    static QString messageRemovalRecordsRemovedSig();

    static QString retrievalInProgressSig();
    static QString transmissionInProgressSig();

    static const int maxNotifySegmentSize = 0;

public slots:
    void processIpcMessageQueue();
    void ipcMessage(const QString& message, const QByteArray& data);
    void flushNotifications();
    void aboutToQuit();
    void ipcConnectionFailed();
    void lockFileUpdated();

protected:
    typedef void (QMailStore::*AccountUpdateSignal)(const QMailAccountIdList&);
    typedef QMap<QString, AccountUpdateSignal> AccountUpdateSignalMap;
    static AccountUpdateSignalMap initAccountUpdateSignals();

    typedef void (QMailStore::*FolderUpdateSignal)(const QMailFolderIdList&);
    typedef QMap<QString, FolderUpdateSignal> FolderUpdateSignalMap;
    static FolderUpdateSignalMap initFolderUpdateSignals();

    typedef void (QMailStore::*ThreadUpdateSignal)(const QMailThreadIdList&);
    typedef QMap<QString, ThreadUpdateSignal> ThreadUpdateSignalMap;
    static ThreadUpdateSignalMap initThreadUpdateSignals();

    typedef void (QMailStore::*MessageUpdateSignal)(const QMailMessageIdList&);
    typedef QMap<QString, MessageUpdateSignal> MessageUpdateSignalMap;
    static MessageUpdateSignalMap initMessageUpdateSignals();

    typedef void (QMailStore::*MessageDataPreCacheSignal)(const QMailMessageMetaDataList&);
    typedef QMap<QString, MessageDataPreCacheSignal> MessageDataPreCacheSignalMap;
    static MessageDataPreCacheSignalMap initMessageDataPreCacheSignals();

    static QMailStore::InitializationState initState;

    virtual void emitIpcNotification(AccountUpdateSignal signal, const QMailAccountIdList &ids);
    virtual void emitIpcNotification(FolderUpdateSignal signal, const QMailFolderIdList &ids);
    virtual void emitIpcNotification(ThreadUpdateSignal signal, const QMailThreadIdList &ids);
    virtual void emitIpcNotification(MessageUpdateSignal signal, const QMailMessageIdList &ids);
    virtual void emitIpcNotification(MessageDataPreCacheSignal signal, const QMailMessageMetaDataList &data);
    virtual void emitIpcNotification(const QMailMessageIdList& ids,  const QMailMessageKey::Properties& properties,
                                     const QMailMessageMetaData& data);
    virtual void emitIpcNotification(const QMailMessageIdList& ids, quint64 status, bool set);

private:
    virtual bool initStore() = 0;

    bool emitIpcNotification();

    QMailStore* q;
    
    mutable QMailStore::ErrorCode errorCode;

    bool asyncEmission;

    QTimer preFlushTimer;
    QTimer flushTimer;

    QSet<QMailAccountId> addAccountsBuffer;
    QSet<QMailFolderId> addFoldersBuffer;
    QSet<QMailThreadId> addThreadsBuffer;
    QSet<QMailMessageId> addMessagesBuffer;
    QSet<QMailAccountId> addMessageRemovalRecordsBuffer;

    QMailMessageMetaDataList addMessagesDataBuffer;
    QMailMessageMetaDataList updateMessagesDataBuffer;

    typedef QPair<QPair<QMailMessageKey::Properties, QMailMessageMetaData>, QSet<QMailMessageId> > MessagesProperties;
    typedef QList<MessagesProperties> MessagesPropertiesBuffer;
    MessagesPropertiesBuffer messagesPropertiesBuffer;

    typedef QPair<quint64, bool> MessagesStatus;
    typedef QMap<MessagesStatus, QSet<QMailMessageId> > MessagesStatusBuffer;
    MessagesStatusBuffer messagesStatusBuffer;

    QSet<QMailMessageId> updateMessagesBuffer;
    QSet<QMailFolderId> updateFoldersBuffer;
    QSet<QMailThreadId> updateThreadsBuffer;
    QSet<QMailAccountId> updateAccountsBuffer;

    QSet<QMailAccountId> removeMessageRemovalRecordsBuffer;
    QSet<QMailMessageId> removeMessagesBuffer;
    QSet<QMailFolderId> removeFoldersBuffer;
    QSet<QMailThreadId> removeThreadsBuffer;
    QSet<QMailAccountId> removeAccountsBuffer;

    QSet<QMailFolderId> folderContentsModifiedBuffer;
    QSet<QMailThreadId> threadContentsModifiedBuffer;
    QSet<QMailAccountId> accountContentsModifiedBuffer;
    QSet<QMailMessageId> messageContentsModifiedBuffer;

    bool retrievalSetInitialized;
    bool transmissionSetInitialized;

    QSet<QMailAccountId> retrievalInProgressIds;
    QSet<QMailAccountId> transmissionInProgressIds;

    QTimer queueTimer;
    QList<QPair<QString, QByteArray> > messageQueue;
    class QCopChannel* ipcChannel;
    class QFileSystemWatcher* watcher;
};


class QMailStoreImplementation : public QMailStoreImplementationBase
{
public:
    QMailStoreImplementation(QMailStore* parent);

    virtual void clearContent() = 0;

    virtual bool addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                            QMailAccountIdList *addedAccountIds) = 0;

    virtual bool addFolder(QMailFolder *f,
                           QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool addMessages(const QList<QMailMessage *> &m,
                             QMailMessageIdList *addedMessageIds, QMailThreadIdList *addedThreadIds, QMailMessageIdList *updatedMessageIds, QMailThreadIdList *updatedThreadIds, QMailFolderIdList *modifiedFolderIds,  QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool addMessages(const QList<QMailMessageMetaData *> &m,
                             QMailMessageIdList *addedMessageIds, QMailThreadIdList *addedThreadIds, QMailMessageIdList *updatedMessageIds, QMailThreadIdList *updatedThreadIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool addThread(QMailThread *f,
                           QMailThreadIdList *addedThreadIds) = 0;

    virtual bool removeAccounts(const QMailAccountKey &key,
                                QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders, QMailThreadIdList *deletedThreadIds, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                               QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailThreadIdList *deletedThreadIds, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                                QMailMessageIdList *deletedMessages, QMailThreadIdList *deletedThreadIds, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool removeThreads(const QMailThreadKey &key, QMailStore::MessageRemovalOption option,
                               QMailThreadIdList *deletedThreads, QMailMessageIdList *deletedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIdList, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateAccount(QMailAccount *account, QMailAccountConfiguration* config,
                               QMailAccountIdList *updatedAccountIds) = 0;

    virtual bool updateAccountConfiguration(QMailAccountConfiguration* config,
                                            QMailAccountIdList *updatedAccountIds) = 0;

    virtual bool updateFolder(QMailFolder* f,
                              QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &m,
                                QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads, QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data,
                                        QMailMessageIdList *updatedMessageIds, QMailThreadIdList *deletedThreads, QMailThreadIdList *modifiedThreads, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, quint64 messageStatus, bool set,
                                        QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateThread(QMailThread *t,
                              QMailThreadIdList *updatedThreadIds) = 0;

    virtual bool ensureDurability() = 0;

    virtual void lock() = 0;
    virtual void unlock() = 0;

    virtual bool purgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids) = 0;

    virtual int countAccounts(const QMailAccountKey &key) const = 0;
    virtual int countFolders(const QMailFolderKey &key) const = 0;
    virtual int countMessages(const QMailMessageKey &key) const = 0;
    virtual int countThreads(const QMailThreadKey &key) const = 0;

    virtual int sizeOfMessages(const QMailMessageKey &key) const = 0;

    virtual QMailAccountIdList queryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, uint limit, uint offset) const = 0;
    virtual QMailFolderIdList queryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, uint limit, uint offset) const = 0;
    virtual QMailMessageIdList queryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey, uint limit, uint offset) const = 0;
    virtual QMailThreadIdList queryThreads(const QMailThreadKey &key, const QMailThreadSortKey &sortKey, uint limit, uint offset) const = 0;

    virtual QMailAccount account(const QMailAccountId &id) const = 0;
    virtual QMailAccountConfiguration accountConfiguration(const QMailAccountId &id) const = 0;

    virtual QMailFolder folder(const QMailFolderId &id) const = 0;

    virtual QMailMessage message(const QMailMessageId &id) const = 0;
    virtual QMailMessage message(const QString &uid, const QMailAccountId &accountId) const = 0;

    virtual QMailThread thread(const QMailThreadId &id) const = 0;

    virtual QMailMessageMetaData messageMetaData(const QMailMessageId &id) const = 0;
    virtual QMailMessageMetaData messageMetaData(const QString &uid, const QMailAccountId &accountId) const = 0;
    virtual QMailMessageMetaDataList messagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option) const = 0;

    virtual QMailThreadList threads(const QMailThreadKey &key, QMailStore::ReturnOption option) const = 0;

    virtual QMailMessageRemovalRecordList messageRemovalRecords(const QMailAccountId &parentAccountId, const QMailFolderId &parentFolderId) const = 0;

    virtual bool registerAccountStatusFlag(const QString &name) = 0;
    virtual quint64 accountStatusMask(const QString &name) const = 0;

    virtual bool registerFolderStatusFlag(const QString &name) = 0;
    virtual quint64 folderStatusMask(const QString &name) const = 0;

    virtual bool registerMessageStatusFlag(const QString &name) = 0;
    virtual quint64 messageStatusMask(const QString &name) const = 0;
    virtual QMap<QString, QString> messageCustomFields(const QMailMessageId &id) = 0;
};

class QMF_EXPORT QMailStoreNullImplementation : public QMailStoreImplementation
{
public:
    QMailStoreNullImplementation(QMailStore* parent);

    virtual void clearContent();

    virtual bool addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                            QMailAccountIdList *addedAccountIds);

    virtual bool addFolder(QMailFolder *f,
                           QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool addMessages(const QList<QMailMessage *> &m,
                             QMailMessageIdList *addedMessageIds, QMailThreadIdList *addedThreadIds, QMailMessageIdList *updatedMessageIds, QMailThreadIdList *updatedThreadIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool addMessages(const QList<QMailMessageMetaData *> &m,
                             QMailMessageIdList *addedMessageIds, QMailThreadIdList *addedThreadIds, QMailMessageIdList *updatedMessageIds, QMailThreadIdList *updatedThreadIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool addThread(QMailThread *t,
                               QMailThreadIdList *addedThreadIds);

    virtual bool removeAccounts(const QMailAccountKey &key,
                                QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders, QMailThreadIdList *deletedThreadIds, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                               QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailThreadIdList *deletedThreadIds, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                                QMailMessageIdList *deletedMessages, QMailThreadIdList* deletedThreadIds, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool removeThreads(const QMailThreadKey &key, QMailStore::MessageRemovalOption option,
                               QMailThreadIdList *deletedThreads, QMailMessageIdList *deletedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIdList, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateAccount(QMailAccount *account, QMailAccountConfiguration* config,
                               QMailAccountIdList *updatedAccountIds);

    virtual bool updateAccountConfiguration(QMailAccountConfiguration* config,
                                            QMailAccountIdList *updatedAccountIds);

    virtual bool updateFolder(QMailFolder* f,
                              QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &m,
                                QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads, QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data,
                                        QMailMessageIdList *updatedMessageIds, QMailThreadIdList *deletedThreads, QMailThreadIdList *modifiedThreads, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, quint64 messageStatus, bool set,
                                        QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateThread(QMailThread *t,
                                 QMailThreadIdList *updatedThreadIds);

    virtual bool ensureDurability();

    virtual void lock();
    virtual void unlock();

    virtual bool purgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids);

    virtual int countAccounts(const QMailAccountKey &key) const;
    virtual int countFolders(const QMailFolderKey &key) const;
    virtual int countMessages(const QMailMessageKey &key) const;
    virtual int countThreads(const QMailThreadKey &key) const;

    virtual int sizeOfMessages(const QMailMessageKey &key) const;

    virtual QMailAccountIdList queryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, uint limit, uint offset) const;
    virtual QMailFolderIdList queryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, uint limit, uint offset) const;
    virtual QMailMessageIdList queryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey, uint limit, uint offset) const;
    virtual QMailThreadIdList queryThreads(const QMailThreadKey &key, const QMailThreadSortKey &sortKey, uint limit, uint offset) const;

    virtual QMailAccount account(const QMailAccountId &id) const;
    virtual QMailAccountConfiguration accountConfiguration(const QMailAccountId &id) const;

    virtual QMailFolder folder(const QMailFolderId &id) const;

    virtual QMailMessage message(const QMailMessageId &id) const;
    virtual QMailMessage message(const QString &uid, const QMailAccountId &accountId) const;

    virtual QMailThread thread(const QMailThreadId &id) const;

    virtual QMailMessageMetaData messageMetaData(const QMailMessageId &id) const;
    virtual QMailMessageMetaData messageMetaData(const QString &uid, const QMailAccountId &accountId) const;
    virtual QMailMessageMetaDataList messagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option) const;

    virtual QMailThreadList threads(const QMailThreadKey &key, QMailStore::ReturnOption option) const;

    virtual QMailMessageRemovalRecordList messageRemovalRecords(const QMailAccountId &parentAccountId, const QMailFolderId &parentFolderId) const;

    virtual bool registerAccountStatusFlag(const QString &name);
    virtual quint64 accountStatusMask(const QString &name) const;

    virtual bool registerFolderStatusFlag(const QString &name);
    virtual quint64 folderStatusMask(const QString &name) const;

    virtual bool registerMessageStatusFlag(const QString &name);
    virtual quint64 messageStatusMask(const QString &name) const;

    virtual QMap<QString, QString> messageCustomFields(const QMailMessageId &id);

private:
    virtual bool initStore();
};

#endif

