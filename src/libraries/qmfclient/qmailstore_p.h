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

#ifndef QMAILSTORE_P_H
#define QMAILSTORE_P_H

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

#include "qmailstoreimplementation_p.h"
#include "qmailstoresql_p.h"
#include "qmailstoreaccount.h"
#include <QCache>
#include <QTimer>

class QMailStorePrivate : public QMailStoreImplementation, public QMailStoreSql
{
    Q_OBJECT

public:
    QMailStorePrivate(QMailStore *parent,
                      QMailAccountManager *accountManager = nullptr);
    virtual ~QMailStorePrivate();

    QMailStore::ErrorCode lastError() const override;
    void setLastError(QMailStore::ErrorCode code) const override;

    bool initStore() override;

    void clearContent() override;

    bool addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                    QMailAccountIdList *addedAccountIds) override;

    bool addFolder(QMailFolder *f,
                   QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds) override;

    bool addMessages(const QList<QMailMessage *> &m,
                     QMailMessageIdList *addedMessageIds, QMailThreadIdList *addedThreadIds,
                     QMailMessageIdList *updatedMessageIds, QMailThreadIdList *updatedThreadIds,
                     QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds,
                     QMailAccountIdList *modifiedAccountIds) override;

    bool addMessages(const QList<QMailMessageMetaData *> &m,
                     QMailMessageIdList *addedMessageIds, QMailThreadIdList *addedThreadIds,
                     QMailMessageIdList *updatedMessageIds, QMailThreadIdList *updatedThreadIds,
                     QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds,
                     QMailAccountIdList *modifiedAccountIds) override;

    bool addThread(QMailThread *t,
                   QMailThreadIdList *addedThreadIds) override;

    bool removeAccounts(const QMailAccountKey &key,
                        QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders,
                        QMailThreadIdList *deletedThreadIds, QMailMessageIdList *deletedMessages,
                        QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds,
                        QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds) override;

    bool removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                       QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages,
                       QMailThreadIdList *deletedThreadIds, QMailMessageIdList *updatedMessageIds,
                       QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds,
                       QMailAccountIdList *modifiedAccountIds) override;

    bool removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                        QMailMessageIdList *deletedMessages, QMailThreadIdList* deletedThreadIds,
                        QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds,
                        QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds) override;

    bool removeThreads(const QMailThreadKey &key, QMailStore::MessageRemovalOption option,
                       QMailThreadIdList *deletedThreads, QMailMessageIdList *deletedMessages,
                       QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds,
                       QMailThreadIdList *modifiedThreadIdList, QMailAccountIdList *modifiedAccountIds) override;


    bool updateAccount(QMailAccount *account, QMailAccountConfiguration* config,
                       QMailAccountIdList *updatedAccountIds) override;

    bool updateAccountConfiguration(QMailAccountConfiguration* config,
                                    QMailAccountIdList *updatedAccountIds) override;

    bool updateFolder(QMailFolder* f,
                      QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds) override;

    bool updateThread(QMailThread *t, QMailThreadIdList *updatedThreadIds) override;

    bool updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &m,
                        QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads,
                        QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds,
                        QMailAccountIdList *modifiedAccountIds) override;

    bool updateMessagesMetaData(const QMailMessageKey &key,
                                const QMailMessageKey::Properties &properties,
                                const QMailMessageMetaData &data,
                                QMailMessageIdList *updatedMessageIds,
                                QMailThreadIdList *deletedThreads,
                                QMailThreadIdList *modifiedThreads,
                                QMailFolderIdList *modifiedFolderIds,
                                QMailAccountIdList *modifiedAccountIds) override;

    bool updateMessagesMetaData(const QMailMessageKey &key, quint64 messageStatus, bool set,
                                QMailMessageIdList *updatedMessageIds,
                                QMailThreadIdList *modifiedThreads,
                                QMailFolderIdList *modifiedFolderIds,
                                QMailAccountIdList *modifiedAccountIds) override;

    bool ensureDurability() override;

    void lock() override;
    void unlock() override;

    bool purgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids) override;

    int countAccounts(const QMailAccountKey &key) const override;
    int countFolders(const QMailFolderKey &key) const override;
    int countMessages(const QMailMessageKey &key) const override;
    int countThreads(const QMailThreadKey &key) const override;

    int sizeOfMessages(const QMailMessageKey &key) const override;

    QMailAccountIdList queryAccounts(const QMailAccountKey &key,
                                     const QMailAccountSortKey &sortKey,
                                     uint limit, uint offset) const override;
    QMailFolderIdList queryFolders(const QMailFolderKey &key,
                                   const QMailFolderSortKey &sortKey,
                                   uint limit, uint offset) const override;
    QMailMessageIdList queryMessages(const QMailMessageKey &key,
                                     const QMailMessageSortKey &sortKey,
                                     uint limit, uint offset) const override;
    QMailThreadIdList queryThreads(const QMailThreadKey &key,
                                   const QMailThreadSortKey &sortKey,
                                   uint limit, uint offset) const override;

    QMailAccount account(const QMailAccountId &id) const override;
    QMailAccountConfiguration accountConfiguration(const QMailAccountId &id) const override;

    QMailFolder folder(const QMailFolderId &id) const override;

    QMailMessage message(const QMailMessageId &id) const override;
    QMailMessage message(const QString &uid, const QMailAccountId &accountId) const override;

    QMailThread thread(const QMailThreadId &id) const override;

    QMailMessageMetaData messageMetaData(const QMailMessageId &id) const override;
    QMailMessageMetaData messageMetaData(const QString &uid, const QMailAccountId &accountId) const override;
    QMailMessageMetaDataList messagesMetaData(const QMailMessageKey &key,
                                              const QMailMessageKey::Properties &properties,
                                              QMailStore::ReturnOption option) const override;

    QMailThreadList threads(const QMailThreadKey &key, QMailStore::ReturnOption option) const override;

    QMailMessageRemovalRecordList messageRemovalRecords(const QMailAccountId &parentAccountId,
                                                        const QMailFolderId &parentFolderId) const override;

    bool registerAccountStatusFlag(const QString &name) override;
    quint64 accountStatusMask(const QString &name) const override;

    bool registerFolderStatusFlag(const QString &name) override;
    quint64 folderStatusMask(const QString &name) const override;

    bool registerMessageStatusFlag(const QString &name) override;
    quint64 messageStatusMask(const QString &name) const override;

    QMap<QString, QString> messageCustomFields(const QMailMessageId &id) override;

    void disconnectIpc() override;
    void reconnectIpc() override;

private:
    void errorChanged() const override;
    void databaseOpened() const override;
    void unloadDatabase();
    void updateMessageValues(const QMailMessageKey::Properties& properties,
                             const QMailMessageMetaData &values,
                             QMailMessageMetaData *metaData) const;
    void removeExpiredData(const QMailMessageIdList& messageIds,
                           const QMailThreadIdList& threadIds,
                           const QMailFolderIdList& folderIds = QMailFolderIdList(),
                           const QMailAccountIdList& accountIds = QMailAccountIdList());
    void preloadHeaderCache(const QMailMessageId& id) const;
    void preloadThreadCache(const QMailThreadId& id) const;

    void accountsRemotelyChanged(QMailStore::ChangeType changeType,
                                 const QMailAccountIdList& ids) override;
    void messageRemovalRecordsRemotelyChanged(QMailStore::ChangeType changeType,
                                              const QMailAccountIdList& ids) override;
    void remoteTransmissionInProgress(const QMailAccountIdList& ids) override;
    void remoteRetrievalInProgress(const QMailAccountIdList& ids) override;
    void messagesRemotelyChanged(QMailStore::ChangeType changeType,
                                 const QMailMessageIdList& ids) override;
    void messageMetaDataRemotelyChanged(QMailStore::ChangeType changeType,
                                        const QMailMessageMetaDataList &data) override;
    void messagePropertiesRemotelyChanged(const QMailMessageIdList& ids,
                                          QMailMessageKey::Properties properties,
                                          const QMailMessageMetaData& data) override;
    void messageStatusRemotelyChanged(const QMailMessageIdList& ids,
                                      quint64 status, bool set) override;
    void threadsRemotelyChanged(QMailStore::ChangeType changeType,
                                const QMailThreadIdList &ids) override;
    void foldersRemotelyChanged(QMailStore::ChangeType changeType,
                                const QMailFolderIdList &ids) override;

    static const int messageCacheSize = 100;
    static const int threadCacheSize = 300;
    static const int uidCacheSize = 500;
    static const int folderCacheSize = 100;
    static const int accountCacheSize = 10;
    static const int lookAhead = 5;

private:
    Q_DECLARE_PUBLIC (QMailStore)
    QMailStore * const q_ptr;
    QMailAccountManager *accountManager = nullptr;
    mutable QTimer databaseUnloadTimer;

    bool externalAccountIdExists(const QMailAccountId &id) const override;
    QMailAccountIdList queryExternalAccounts(const QMailAccountKey &key) const override;

    void onExternalAccountCreated(QMailAccountId id);
    void onExternalAccountRemoved(QMailAccountId id);
    void onExternalAccountUpdated(QMailAccountId id);

    template <typename KeyType, typename T>
    class Cache
    {
    public:
        Cache(unsigned int size = 10);
        ~Cache();

        T lookup(const KeyType& key) const;
        void insert(const KeyType& key, const T& item);
        bool contains(const KeyType& key) const;
        void remove(const KeyType& key);
        void clear();

    private:
        QCache<KeyType, T> mCache;
    };

    template <typename ID, typename T>
    class IdCache : public Cache<quint64, T>
    {
    public:
        IdCache(unsigned int size = 10) : Cache<quint64, T>(size) {}

        T lookup(const ID& id) const;
        void insert(const T& item);
        bool contains(const ID& id) const;
        void remove(const ID& id);
    };

    typedef QPair<QMailAccountId, QString> ServerUid;

    mutable IdCache<QMailMessageId, QMailMessageMetaData> messageCache;
    mutable Cache<ServerUid, QMailMessageId> uidCache;
    mutable IdCache<QMailFolderId, QMailFolder> folderCache;
    mutable IdCache<QMailAccountId, QMailAccount> accountCache;
    mutable IdCache<QMailThreadId, QMailThread> threadCache;

    mutable QMailMessageIdList lastQueryMessageResult;
    mutable QMailThreadIdList lastQueryThreadResult;

    QDateTime ipcLastDbUpdated;
};

template <typename KeyType, typename T>
QMailStorePrivate::Cache<KeyType, T>::Cache(unsigned int cacheSize)
    : mCache(cacheSize)
{
}

template <typename KeyType, typename T>
QMailStorePrivate::Cache<KeyType, T>::~Cache()
{
}

template <typename KeyType, typename T>
T QMailStorePrivate::Cache<KeyType, T>::lookup(const KeyType& key) const
{
    if (T* cachedItem = mCache.object(key))
        return *cachedItem;

    return T();
}

template <typename KeyType, typename T>
void QMailStorePrivate::Cache<KeyType, T>::insert(const KeyType& key, const T& item)
{
    mCache.insert(key, new T(item));
}

template <typename KeyType, typename T>
bool QMailStorePrivate::Cache<KeyType, T>::contains(const KeyType& key) const
{
    return mCache.contains(key);
}

template <typename KeyType, typename T>
void QMailStorePrivate::Cache<KeyType, T>::remove(const KeyType& key)
{
    mCache.remove(key);
}

template <typename KeyType, typename T>
void QMailStorePrivate::Cache<KeyType, T>::clear()
{
    mCache.clear();
}


template <typename ID, typename T>
T QMailStorePrivate::IdCache<ID, T>::lookup(const ID& id) const
{
    if (id.isValid()) {
        return Cache<quint64, T>::lookup(id.toULongLong());
    }

    return T();
}

template <typename ID, typename T>
void QMailStorePrivate::IdCache<ID, T>::insert(const T& item)
{
    if (item.id().isValid()) {
        Cache<quint64, T>::insert(item.id().toULongLong(), item);
    }
}

template <typename ID, typename T>
bool QMailStorePrivate::IdCache<ID, T>::contains(const ID& id) const
{
    return Cache<quint64, T>::contains(id.toULongLong());
}

template <typename ID, typename T>
void QMailStorePrivate::IdCache<ID, T>::remove(const ID& id)
{
    Cache<quint64, T>::remove(id.toULongLong());
}

#endif
