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

#include "qmailstore_p.h"
#include "qmailcontentmanager.h"
#include "qmailmessageremovalrecord.h"
#include "qmailtimestamp.h"
#include "qmailnamespace.h"
#include "qmaillog.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextCodec>
#include <QThread>
#include <QRegularExpression>

QMailStorePrivate::QMailStorePrivate(QMailStore* parent,
                                     QMailAccountManager *accountManager)
    : QMailStoreImplementation(parent),
      QMailStoreSql(accountManager == nullptr),
      q_ptr(parent),
      accountManager(accountManager),
      messageCache(messageCacheSize),
      uidCache(uidCacheSize),
      folderCache(folderCacheSize),
      accountCache(accountCacheSize),
      threadCache(threadCacheSize)
{
    connect(&databaseUnloadTimer, &QTimer::timeout,
            this, &QMailStorePrivate::unloadDatabase);

    if (accountManager) {
        connect(accountManager, &QMailAccountManager::accountCreated,
                this, &QMailStorePrivate::onExternalAccountCreated);
        connect(accountManager, &QMailAccountManager::accountRemoved,
                this, &QMailStorePrivate::onExternalAccountRemoved);
        connect(accountManager, &QMailAccountManager::accountUpdated,
                this, &QMailStorePrivate::onExternalAccountUpdated);
    }
}

QMailStorePrivate::~QMailStorePrivate()
{
}

void QMailStorePrivate::onExternalAccountCreated(QMailAccountId id)
{
    accountsRemotelyChanged(QMailStore::Added, QMailAccountIdList() << id);
}

void QMailStorePrivate::onExternalAccountRemoved(QMailAccountId id)
{
    accountsRemotelyChanged(QMailStore::Removed, QMailAccountIdList() << id);
}

void QMailStorePrivate::onExternalAccountUpdated(QMailAccountId id)
{
    accountsRemotelyChanged(QMailStore::Updated, QMailAccountIdList() << id);
}

bool QMailStorePrivate::initStore()
{
    if (!QMailStoreSql::initStore(tr("Local Storage"))) {
        q_ptr->emitErrorNotification(lastError());
        return false;
    }
    return true;
}

QMailStore::ErrorCode QMailStorePrivate::lastError() const
{
    return QMailStoreSql::lastError();
}

void QMailStorePrivate::setLastError(QMailStore::ErrorCode code) const
{
    if (lastError() != code) {
        QMailStoreSql::setLastError(code);
        q_ptr->emitErrorNotification(code);
    }
}

void QMailStorePrivate::errorChanged() const
{
    q_ptr->emitErrorNotification(lastError());
}

void QMailStorePrivate::clearContent()
{
    // Clear all caches
    accountCache.clear();
    folderCache.clear();
    messageCache.clear();
    uidCache.clear();
    threadCache.clear();
    lastQueryMessageResult.clear();
    lastQueryThreadResult.clear();

    QMailStoreSql::clearContent();
    if (accountManager) {
        accountManager->clearContent();
    }
}

QMap<QString, QString> QMailStorePrivate::messageCustomFields(const QMailMessageId &id)
{
    return QMailStoreSql::messageCustomFields(id);
}

bool QMailStorePrivate::addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                                   QMailAccountIdList *addedAccountIds)
{
    if (accountManager) {
        // Don't populate addedAccountIds,
        // notification will be sent by the account manager.
        return accountManager->addAccount(account, config)
            && QMailStoreSql::setAccountStandardFolders(account->id(),
                                                        account->standardFolders());
    } else {
        return QMailStoreSql::addAccount(account, config, addedAccountIds);
    }
}

bool QMailStorePrivate::addFolder(QMailFolder *folder,
                                  QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    return QMailStoreSql::addFolder(folder, addedFolderIds, modifiedAccountIds);
}

bool QMailStorePrivate::addMessages(const QList<QMailMessage *> &messages,
                                    QMailMessageIdList *addedMessageIds, QMailThreadIdList *addedThreadIds, QMailMessageIdList *updatedMessageIds, QMailThreadIdList *updatedThreadIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds)
{
    return QMailStoreSql::addMessages(messages, addedMessageIds, addedThreadIds, updatedMessageIds, updatedThreadIds, modifiedFolderIds, modifiedThreadIds, modifiedAccountIds);
}

bool QMailStorePrivate::addMessages(const QList<QMailMessageMetaData *> &messages,
                                    QMailMessageIdList *addedMessageIds, QMailThreadIdList *addedThreadIds, QMailMessageIdList *updatedMessageIds, QMailThreadIdList *updatedThreadIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds)
{
    return QMailStoreSql::addMessages(messages, addedMessageIds, addedThreadIds, updatedMessageIds, updatedThreadIds, modifiedFolderIds, modifiedThreadIds, modifiedAccountIds);
}

bool QMailStorePrivate::addThread(QMailThread *thread, QMailThreadIdList *addedThreadIds)
{
    return QMailStoreSql::addThread(thread, addedThreadIds);
}

bool QMailStorePrivate::removeAccounts(const QMailAccountKey &key,
                                       QMailAccountIdList *deletedAccountIds, QMailFolderIdList *deletedFolderIds, QMailThreadIdList *deletedThreadIds, QMailMessageIdList *deletedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds)
{
    bool success = QMailStoreSql::removeAccounts(key, deletedAccountIds, deletedFolderIds, deletedThreadIds, deletedMessageIds, updatedMessageIds, modifiedFolderIds, modifiedThreadIds, modifiedAccountIds);
    if (success) {
        removeExpiredData(*deletedMessageIds, *deletedThreadIds, *deletedFolderIds, *deletedAccountIds);
        if (accountManager) {
            success = accountManager->removeAccounts(*deletedAccountIds);
            // Don't populate deletedAccountIds,
            // notification will be sent by the account manager.
            deletedAccountIds->clear();
        }
    }
    return success;
}

bool QMailStorePrivate::removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                                      QMailFolderIdList *deletedFolderIds, QMailMessageIdList *deletedMessageIds, QMailThreadIdList *deletedThreadIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds)
{
    bool success = QMailStoreSql::removeFolders(key, option, deletedFolderIds, deletedMessageIds, deletedThreadIds, updatedMessageIds, modifiedFolderIds, modifiedThreadIds, modifiedAccountIds);
    if (success) {
        removeExpiredData(*deletedMessageIds, *deletedThreadIds, *deletedFolderIds);
    }
    return success;
}

bool QMailStorePrivate::removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                                       QMailMessageIdList *deletedMessageIds, QMailThreadIdList* deletedThreadIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds)
{
    bool success = QMailStoreSql::removeMessages(key, option, deletedMessageIds, deletedThreadIds, updatedMessageIds, modifiedFolderIds, modifiedThreadIds, modifiedAccountIds);
    if (success) {
        removeExpiredData(*deletedMessageIds, *deletedThreadIds);
    }
    return success;
}

bool QMailStorePrivate::removeThreads(const QMailThreadKey &key, QMailStore::MessageRemovalOption option,
                               QMailThreadIdList *deletedThreads, QMailMessageIdList *deletedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds)
{
    bool success = QMailStoreSql::removeThreads(key, option, deletedThreads, deletedMessageIds, updatedMessageIds, modifiedFolderIds, modifiedThreadIds, modifiedAccountIds);
    if (success) {
        removeExpiredData(*deletedMessageIds, *deletedThreads);
    }
    return success;
}

bool QMailStorePrivate::updateAccount(QMailAccount *account, QMailAccountConfiguration *config,
                                      QMailAccountIdList *updatedAccountIds)
{
    bool success;
    if (accountManager) {
        // Don't populate updatedAccountIds,
        // notification will be sent by the account manager.
        success = accountManager->updateAccount(account, config)
            && QMailStoreSql::setAccountStandardFolders(account->id(),
                                                        account->standardFolders());
    } else {
        success = QMailStoreSql::updateAccount(account, config, updatedAccountIds);
    }
    if (success) {
        // Update the account cache
        if (accountCache.contains(account->id()))
            accountCache.insert(*account);
    }
    return success;
}

bool QMailStorePrivate::updateAccountConfiguration(QMailAccountConfiguration *config,
                                                   QMailAccountIdList *updatedAccountIds)
{
    if (accountManager) {
        return accountManager->updateAccountConfiguration(config);
    } else {
        return QMailStoreSql::updateAccount(nullptr, config, updatedAccountIds);
    }
}

bool QMailStorePrivate::updateFolder(QMailFolder *folder,
                                     QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    bool success = QMailStoreSql::updateFolder(folder, updatedFolderIds, modifiedAccountIds);
    if (success) {
        //update the folder cache
        if (folderCache.contains(folder->id()))
            folderCache.insert(*folder);
    }
    return success;
}

bool QMailStorePrivate::updateThread(QMailThread *thread,
                                     QMailThreadIdList *updatedThreadIds)
{
    return QMailStoreSql::updateThread(thread, updatedThreadIds);
}

bool QMailStorePrivate::updateMessages(const QList<QPair<QMailMessageMetaData*, QMailMessage*> > &messages,
                                       QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads, QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    bool success = QMailStoreSql::updateMessages(messages, updatedMessageIds, modifiedThreads, modifiedMessageIds, modifiedFolderIds, modifiedAccountIds);
    if (success) {
        for (const QPair<QMailMessageMetaData*, QMailMessage*> &message : messages) {
            QMailMessageMetaData *metaData = message.first;
            if (metaData->parentAccountId().isValid()
                && messageCache.contains(metaData->id())) {
                QMailMessageMetaData cachedMetaData = *metaData;
                // Update the cache
                cachedMetaData.setUnmodified();
                messageCache.insert(cachedMetaData);

                uidCache.insert(qMakePair(metaData->parentAccountId(), metaData->serverUid()), metaData->id());
            }
        }

        foreach (const QMailThreadId& id, *modifiedThreads) {
            threadCache.remove(id);
        }
    }
    return success;
}

void QMailStorePrivate::updateMessageValues(const QMailMessageKey::Properties& properties,
                                            const QMailMessageMetaData &values,
                                            QMailMessageMetaData *metaData) const
{
    QPair<QString, QString> uriElements;

    foreach (QMailMessageKey::Property p, messagePropertyList()) {
        switch (properties & p)
        {
            case QMailMessageKey::Id:
                metaData->setId(values.id());
                break;

            case QMailMessageKey::Type:
                metaData->setMessageType(values.messageType());
                break;

            case QMailMessageKey::ParentFolderId:
                metaData->setParentFolderId(values.parentFolderId());
                break;

            case QMailMessageKey::Sender:
                metaData->setFrom(values.from());
                break;

            case QMailMessageKey::Recipients:
                metaData->setRecipients(values.recipients());
                break;

            case QMailMessageKey::Subject:
                metaData->setSubject(values.subject());
                break;

            case QMailMessageKey::TimeStamp:
                metaData->setDate(values.date());
                break;

            case QMailMessageKey::ReceptionTimeStamp:
                metaData->setReceivedDate(values.receivedDate());
                break;

            case QMailMessageKey::Status:
                metaData->setStatus(values.status());
                break;

            case QMailMessageKey::ParentAccountId:
                metaData->setParentAccountId(values.parentAccountId());
                break;

            case QMailMessageKey::ServerUid:
                metaData->setServerUid(values.serverUid());
                break;

            case QMailMessageKey::Size:
                metaData->setSize(values.size());
                break;

            case QMailMessageKey::ContentType:
                metaData->setContent(values.content());
                break;

            case QMailMessageKey::PreviousParentFolderId:
                metaData->setPreviousParentFolderId(values.previousParentFolderId());
                break;

            case QMailMessageKey::ContentScheme:
                metaData->setContentScheme(values.contentScheme());
                break;

            case QMailMessageKey::ContentIdentifier:
                metaData->setContentIdentifier(values.contentIdentifier());
                break;

            case QMailMessageKey::InResponseTo:
                metaData->setInResponseTo(values.inResponseTo());
                break;

            case QMailMessageKey::ResponseType:
                metaData->setResponseType(values.responseType());
                break;

            case QMailMessageKey::CopyServerUid:
                metaData->setCopyServerUid(values.copyServerUid());
                break;

            case QMailMessageKey::RestoreFolderId:
                metaData->setRestoreFolderId((values.restoreFolderId()));
                break;

            case QMailMessageKey::ListId:
                metaData->setListId(values.listId());
                break;

            case QMailMessageKey::RfcId:
                metaData->setRfcId(values.rfcId());
                break;

            case QMailMessageKey::Preview:
                metaData->setPreview(values.preview());
                break;

            case QMailMessageKey::ParentThreadId:
                metaData->setParentThreadId(values.parentThreadId());
                break;

            default:
                break;
        }
    }

    // QMailMessageKey::Custom is a "special" kid.
    if ((properties & QMailMessageKey::Custom)) {
        metaData->setCustomFields(values.customFields());
    }

    // The target message is not completely loaded
    metaData->setStatus(QMailMessage::UnloadedData, true);
}

bool QMailStorePrivate::updateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data,
                                               QMailMessageIdList *updatedMessageIds, QMailThreadIdList *deletedThreads, QMailThreadIdList *modifiedThreads, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    bool success = QMailStoreSql::updateMessagesMetaData(key, properties, data, updatedMessageIds, deletedThreads, modifiedThreads, modifiedFolderIds, modifiedAccountIds);
    if (success) {
        // Update the header cache
        foreach (const QMailMessageId& id, *updatedMessageIds) {
            if (messageCache.contains(id)) {
                QMailMessageMetaData cachedMetaData = messageCache.lookup(id);
                updateMessageValues(properties, data, &cachedMetaData);
                cachedMetaData.setUnmodified();
                messageCache.insert(cachedMetaData);
                uidCache.insert(qMakePair(cachedMetaData.parentAccountId(), cachedMetaData.serverUid()), cachedMetaData.id());
            }
        }

        foreach (const QMailThreadId& id, *modifiedThreads) {
            threadCache.remove(id);
        }
    }
    return success;
}

bool QMailStorePrivate::updateMessagesMetaData(const QMailMessageKey &key, quint64 status, bool set,
                                               QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    bool success = QMailStoreSql::updateMessagesMetaData(key, status, set, updatedMessageIds, modifiedThreads, modifiedFolderIds, modifiedAccountIds);
    if (success) {
        // Update the header cache
        foreach (const QMailMessageId& id, *updatedMessageIds) {
            if (messageCache.contains(id)) {
                QMailMessageMetaData cachedMetaData = messageCache.lookup(id);
                quint64 newStatus = cachedMetaData.status();
                newStatus = set ? (newStatus | status) : (newStatus & ~status);
                cachedMetaData.setStatus(newStatus);
                cachedMetaData.setUnmodified();
                messageCache.insert(cachedMetaData);
                uidCache.insert(qMakePair(cachedMetaData.parentAccountId(), cachedMetaData.serverUid()), cachedMetaData.id());
            }
        }

        foreach (const QMailThreadId& id, *modifiedThreads) {
            threadCache.remove(id);
        }
    }
    return success;
}

bool QMailStorePrivate::ensureDurability()
{
    return QMailStoreSql::ensureDurability();
}

void QMailStorePrivate::databaseOpened() const
{
    databaseUnloadTimer.start(QMail::databaseAutoCloseTimeout());
}

void QMailStorePrivate::unloadDatabase()
{
    databaseUnloadTimer.stop();
    QMailStoreSql::unloadDatabase();
    // Clear all caches
    accountCache.clear();
    folderCache.clear();
    messageCache.clear();
    uidCache.clear();
    threadCache.clear();
    lastQueryMessageResult.clear();
    lastQueryThreadResult.clear();
}

void QMailStorePrivate::lock()
{
    QMailStoreSql::lock();
}

void QMailStorePrivate::unlock()
{
    QMailStoreSql::unlock();
}

bool QMailStorePrivate::purgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids)
{
    return QMailStoreSql::purgeMessageRemovalRecords(accountId, serverUids);
}

int QMailStorePrivate::countAccounts(const QMailAccountKey &key) const
{
    return accountManager ? accountManager->countAccounts(key)
        : QMailStoreSql::countAccounts(key);
}

int QMailStorePrivate::countFolders(const QMailFolderKey &key) const
{
    return QMailStoreSql::countFolders(key);
}

int QMailStorePrivate::countMessages(const QMailMessageKey &key) const
{
    return QMailStoreSql::countMessages(key);
}

int QMailStorePrivate::countThreads(const QMailThreadKey &key) const
{
    return QMailStoreSql::countThreads(key);
}

int QMailStorePrivate::sizeOfMessages(const QMailMessageKey &key) const
{
    return QMailStoreSql::sizeOfMessages(key);
}

bool QMailStorePrivate::externalAccountIdExists(const QMailAccountId &id) const
{
    return accountManager && accountManager->account(id).id().isValid();
}

QMailAccountIdList QMailStorePrivate::queryExternalAccounts(const QMailAccountKey &key) const
{
    return accountManager ? accountManager->queryAccounts(key) : QMailAccountIdList();
}

QMailAccountIdList QMailStorePrivate::queryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, uint limit, uint offset) const
{
    return accountManager ? accountManager->queryAccounts(key, sortKey, limit, offset)
        : QMailStoreSql::queryAccounts(key, sortKey, limit, offset);
}

QMailFolderIdList QMailStorePrivate::queryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, uint limit, uint offset) const
{
    return QMailStoreSql::queryFolders(key, sortKey, limit, offset);
}

QMailMessageIdList QMailStorePrivate::queryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey, uint limit, uint offset) const
{
    //store the results of this call for cache preloading
    lastQueryMessageResult = QMailStoreSql::queryMessages(key, sortKey, limit, offset);
    return lastQueryMessageResult;
}

QMailThreadIdList QMailStorePrivate::queryThreads(const QMailThreadKey &key, const QMailThreadSortKey &sortKey, uint limit, uint offset) const
{
    //store the results of this call for cache preloading
    lastQueryThreadResult = QMailStoreSql::queryThreads(key, sortKey, limit, offset);
    return lastQueryThreadResult;
}

QMailAccount QMailStorePrivate::account(const QMailAccountId &id) const
{
    if (accountCache.contains(id))
        return accountCache.lookup(id);
    QMailAccount result(accountManager
                        ? accountManager->account(id)
                        : QMailStoreSql::account(id));
    if (result.id().isValid()) {
        if (accountManager) {
            const QMap<QMailFolder::StandardFolder, QMailFolderId> folders
                = QMailStoreSql::accountStandardFolders(id);
            QMap<QMailFolder::StandardFolder, QMailFolderId>::ConstIterator it;
            for (it = folders.constBegin(); it != folders.constEnd(); it++) {
                result.setStandardFolder(it.key(), it.value());
            }
        }
        //update cache
        accountCache.insert(result);
    }
    return result;
}

QMailAccountConfiguration QMailStorePrivate::accountConfiguration(const QMailAccountId &id) const
{
    if (accountManager) {
        const QMailAccountConfiguration config(accountManager->accountConfiguration(id));
        if (!config.id().isValid()) {
            setLastError(QMailStore::InvalidId);
        }
        return config;
    } else {
        return QMailStoreSql::accountConfiguration(id);
    }
}

QMailFolder QMailStorePrivate::folder(const QMailFolderId &id) const
{
    if (folderCache.contains(id))
        return folderCache.lookup(id);

    QMailFolder result = QMailStoreSql::folder(id);
    if (result.id().isValid()) {
        //update cache
        folderCache.insert(result);
    }
    return result;
}

QMailMessage QMailStorePrivate::message(const QMailMessageId &id) const
{
    return QMailStoreSql::message(id);
}

QMailMessage QMailStorePrivate::message(const QString &uid, const QMailAccountId &accountId) const
{
    return QMailStoreSql::message(uid, accountId);
}

QMailThread QMailStorePrivate::thread(const QMailThreadId &id) const
{
    if (threadCache.contains(id))
        return threadCache.lookup(id);

    // if not in the cache, then preload the cache with the id and its most likely requested siblings
    preloadThreadCache(id);

    return threadCache.lookup(id);
}

QMailMessageMetaData QMailStorePrivate::messageMetaData(const QMailMessageId &id) const
{
    if (messageCache.contains(id)) {
        return messageCache.lookup(id);
    }

    //if not in the cache, then preload the cache with the id and its most likely requested siblings
    preloadHeaderCache(id);

    return messageCache.lookup(id);
}

QMailMessageMetaData QMailStorePrivate::messageMetaData(const QString &uid, const QMailAccountId &accountId) const
{
    QMailMessageMetaData metaData;

    ServerUid key(accountId, uid);
    if (uidCache.contains(key)) {
        // We can look this message up in the cache
        QMailMessageId id(uidCache.lookup(key));

        if (messageCache.contains(id))
            return messageCache.lookup(id);

        metaData = QMailStoreSql::messageMetaData(id);
    } else {
        metaData = QMailStoreSql::messageMetaData(uid, accountId);
    }

    if (metaData.id().isValid()) {
        messageCache.insert(metaData);
        uidCache.insert(qMakePair(metaData.parentAccountId(), metaData.serverUid()), metaData.id());
    }

    return metaData;
}

QMailMessageMetaDataList QMailStorePrivate::messagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option) const
{
    return QMailStoreSql::messagesMetaData(key, properties, option);
}

QMailThreadList QMailStorePrivate::threads(const QMailThreadKey &key, QMailStore::ReturnOption option) const
{
    return QMailStoreSql::threads(key, option);
}

QMailMessageRemovalRecordList QMailStorePrivate::messageRemovalRecords(const QMailAccountId &accountId, const QMailFolderId &folderId) const
{
    return QMailStoreSql::messageRemovalRecords(accountId, folderId);
}

bool QMailStorePrivate::registerAccountStatusFlag(const QString &name)
{
    return QMailStoreSql::registerAccountStatusFlag(name);
}

quint64 QMailStorePrivate::accountStatusMask(const QString &name) const
{
    return QMailStoreSql::accountStatusMask(name);
}

bool QMailStorePrivate::registerFolderStatusFlag(const QString &name)
{
    return QMailStoreSql::registerFolderStatusFlag(name);
}

quint64 QMailStorePrivate::folderStatusMask(const QString &name) const
{
    return QMailStoreSql::folderStatusMask(name);
}

bool QMailStorePrivate::registerMessageStatusFlag(const QString &name)
{
    return QMailStoreSql::registerMessageStatusFlag(name);
}

quint64 QMailStorePrivate::messageStatusMask(const QString &name) const
{
    return QMailStoreSql::messageStatusMask(name);
}

void QMailStorePrivate::removeExpiredData(const QMailMessageIdList& messageIds, const QMailThreadIdList& threadIds, const QMailFolderIdList& folderIds, const QMailAccountIdList& accountIds)
{
    foreach (const QMailMessageId& id, messageIds) {
        messageCache.remove(id);
    }

    foreach (const QMailThreadId& id, threadIds) {
        threadCache.remove(id);
    }

    foreach (const QMailFolderId& id, folderIds) {
        folderCache.remove(id);
    }

    foreach (const QMailAccountId& id, accountIds) {
        accountCache.remove(id);
    }
}

void QMailStorePrivate::preloadHeaderCache(const QMailMessageId& id) const
{
    QMailMessageIdList idBatch;
    idBatch.append(id);

    int index = lastQueryMessageResult.indexOf(id);
    if (index != -1) {
        // Preload based on result of last call to queryMessages
        int count = 1;

        QMailMessageIdList::const_iterator begin = lastQueryMessageResult.begin();
        QMailMessageIdList::const_iterator end = lastQueryMessageResult.end();
        QMailMessageIdList::const_iterator lowIt = begin + index;
        QMailMessageIdList::const_iterator highIt = lowIt;

        bool ascend(true);
        bool descend(lowIt != begin);

        while ((count < (QMailStorePrivate::lookAhead * 2)) && (ascend || descend)) {
            if (ascend) {
                ++highIt;
                if (highIt == end) {
                    ascend = false;
                } else  {
                    if (!messageCache.contains(*highIt)) {
                        idBatch.append(*highIt);
                        ++count;
                    } else {
                        // Most likely, a sequence in the other direction will be more useful
                        ascend = false;
                    }
                }
            }

            if (descend) {
                --lowIt;
                if (!messageCache.contains(*lowIt)) {
                    idBatch.prepend(*lowIt);
                    ++count;

                    if (lowIt == begin) {
                        descend = false;
                    }
                } else {
                    // Most likely, a sequence in the other direction will be more useful
                    descend = false;
                }
            }
        }
    } else {
        // Don't bother preloading - if there is a query result, we have now searched outside it;
        // we should consider it to have outlived its usefulness
        if (!lastQueryMessageResult.isEmpty())
            lastQueryMessageResult = QMailMessageIdList();
    }

    QMailMessageMetaData result;
    QMailMessageKey key(QMailMessageKey::id(idBatch));
    for (const QMailMessageMetaData& metaData : messagesMetaData(key, allMessageProperties(), QMailStore::ReturnAll)) {
        if (metaData.id().isValid()) {
            messageCache.insert(metaData);
            uidCache.insert(qMakePair(metaData.parentAccountId(), metaData.serverUid()), metaData.id());
            if (metaData.id() == id) {
                result = metaData;
        }
    }
    }
}

void QMailStorePrivate::preloadThreadCache(const QMailThreadId& id) const
{
    QMailThreadIdList idBatch;
    idBatch.append(id);

    int index = lastQueryThreadResult.indexOf(id);
    if (index == -1) {
        // Don't bother preloading - if there is a query result, we have now searched outside it;
        // we should consider it to have outlived its usefulness
        if (!lastQueryThreadResult.isEmpty())
            lastQueryThreadResult = QMailThreadIdList();
    } else {

        // Preload based on result of last call to queryMessages
        int count = 1;

        QMailThreadIdList::const_iterator begin = lastQueryThreadResult.begin();
        QMailThreadIdList::const_iterator end = lastQueryThreadResult.end();
        QMailThreadIdList::const_iterator lowIt = begin + index;
        QMailThreadIdList::const_iterator highIt = lowIt;

        bool ascend(true);
        bool descend(lowIt != begin);

        while ((count < (QMailStorePrivate::lookAhead * 2)) && (ascend || descend)) {
            if (ascend) {
                ++highIt;
                if (highIt == end) {
                    ascend = false;
                } else  {
                    if (!threadCache.contains(*highIt)) {
                        idBatch.append(*highIt);
                        ++count;
                    } else {
                        // Most likely, a sequence in the other direction will be more useful
                        ascend = false;
                    }
                }
            }

            if (descend) {
                --lowIt;
                if (!threadCache.contains(*lowIt)) {
                    idBatch.prepend(*lowIt);
                    ++count;

                    if (lowIt == begin) {
                        descend = false;
                    }
                } else {
                    // Most likely, a sequence in the other direction will be more useful
                    descend = false;
                }
            }
        }
    }

    QMailThread result;
    QMailThreadKey key(QMailThreadKey::id(idBatch));
    for (const QMailThread &thread : threads(key, QMailStore::ReturnAll)) {
        if (thread.id().isValid()) {
            threadCache.insert(thread);
            if (thread.id() == id)
                result = thread;
            // TODO: populate uid cache
        }
    }
}

void QMailStorePrivate::accountsRemotelyChanged(QMailStore::ChangeType changeType,
                                                const QMailAccountIdList& ids)
{
    if ((changeType == QMailStore::Updated)
        || (changeType == QMailStore::Removed)) {
        for (const QMailAccountId &id : ids)
            accountCache.remove(id);
    }

    switch (changeType) {
    case QMailStore::Added:
        emit q_ptr->accountsAdded(ids);
        break;
    case QMailStore::Removed:
        emit q_ptr->accountsRemoved(ids);
        break;
    case QMailStore::Updated:
        emit q_ptr->accountsUpdated(ids);
        break;
    case QMailStore::ContentsModified:
        emit q_ptr->accountContentsModified(ids);
        break;
    default:
        qWarning() << "Unhandled remote account notification";
        break;
    }
}

void QMailStorePrivate::messageRemovalRecordsRemotelyChanged(QMailStore::ChangeType changeType,
                                                             const QMailAccountIdList& ids)
{
    switch (changeType) {
    case QMailStore::Added:
        emit q_ptr->messageRemovalRecordsAdded(ids);
        break;
    case QMailStore::Removed:
        emit q_ptr->messageRemovalRecordsRemoved(ids);
        break;
    default:
        qWarning() << "Unhandled remote message removal record notification";
        break;
    }
}

void QMailStorePrivate::remoteTransmissionInProgress(const QMailAccountIdList& ids)
{
    emit q_ptr->transmissionInProgress(ids);
}

void QMailStorePrivate::remoteRetrievalInProgress(const QMailAccountIdList& ids)
{
    emit q_ptr->retrievalInProgress(ids);
}

void QMailStorePrivate::foldersRemotelyChanged(QMailStore::ChangeType changeType,
                                               const QMailFolderIdList &ids)
{
    if ((changeType == QMailStore::Updated)
        || (changeType == QMailStore::Removed)) {
        for (const QMailFolderId &id : ids)
            folderCache.remove(id);
    }

    switch (changeType) {
    case QMailStore::Added:
        emit q_ptr->foldersAdded(ids);
        break;
    case QMailStore::Removed:
        emit q_ptr->foldersRemoved(ids);
        break;
    case QMailStore::Updated:
        emit q_ptr->foldersUpdated(ids);
        break;
    case QMailStore::ContentsModified:
        emit q_ptr->folderContentsModified(ids);
        break;
    default:
        qWarning() << "Unhandled remote folder notification";
        break;
    }
}

void QMailStorePrivate::threadsRemotelyChanged(QMailStore::ChangeType changeType,
                                               const QMailThreadIdList &ids)
{
    if ((changeType == QMailStore::Updated)
        || (changeType == QMailStore::Removed)) {
        for (const QMailThreadId &id : ids)
            threadCache.remove(id);
    }

    switch (changeType) {
    case QMailStore::Added:
        emit q_ptr->threadsAdded(ids);
        break;
    case QMailStore::Removed:
        emit q_ptr->threadsRemoved(ids);
        break;
    case QMailStore::Updated:
        emit q_ptr->threadsUpdated(ids);
        break;
    case QMailStore::ContentsModified:
        emit q_ptr->threadContentsModified(ids);
        break;
    default:
        qWarning() << "Unhandled remote thread notification";
        break;
    }
}

void QMailStorePrivate::messagesRemotelyChanged(QMailStore::ChangeType changeType,
                                                const QMailMessageIdList& ids)
{
    if ((changeType == QMailStore::Updated)
        || (changeType == QMailStore::Removed)) {
        for (const QMailMessageId &id : ids)
            messageCache.remove(id);
    }

    switch (changeType) {
    case QMailStore::Added:
        emit q_ptr->messagesAdded(ids);
        break;
    case QMailStore::Removed:
        emit q_ptr->messagesRemoved(ids);
        break;
    case QMailStore::Updated:
        emit q_ptr->messagesUpdated(ids);
        break;
    case QMailStore::ContentsModified:
        emit q_ptr->messageContentsModified(ids);
        break;
    default:
        qWarning() << "Unhandled remote account notification";
        break;
    }
}

void QMailStorePrivate::messageMetaDataRemotelyChanged(QMailStore::ChangeType changeType,
                                                       const QMailMessageMetaDataList &data)
{
    if(!data.isEmpty()) {

        QMailMessageIdList ids;

        for (const QMailMessageMetaData& metaData : data)
        {
            messageCache.insert(metaData);
            uidCache.insert(qMakePair(metaData.parentAccountId(), metaData.serverUid()), metaData.id());

            ids.append(metaData.id());
        }

        switch (changeType) {
        case QMailStore::Added:
            emit q_ptr->messageDataAdded(data);
            break;
        case QMailStore::Updated:
            emit q_ptr->messageDataUpdated(data);
            break;
        default:
            qWarning() << "Unhandled remote message meta data notification";
            break;
        }
    }

}

void QMailStorePrivate::messagePropertiesRemotelyChanged(const QMailMessageIdList& ids,
                                                         QMailMessageKey::Properties properties,
                                                         const QMailMessageMetaData& data)
{
    Q_ASSERT(!ids.contains(QMailMessageId()));

    foreach(const QMailMessageId& id, ids) {

        if(messageCache.contains(id)) {
            QMailMessageMetaData metaData = messageCache.lookup(id);
            if ((properties & QMailMessageKey::Custom)) {
                metaData.setCustomFields(data.customFields());
            }
            foreach (QMailMessageKey::Property p, messagePropertyList()) {
                switch (properties & p)
                {
                case QMailMessageKey::Id:
                    metaData.setId(data.id());
                    break;

                case QMailMessageKey::Type:
                    metaData.setMessageType(data.messageType());
                    break;

                case QMailMessageKey::ParentFolderId:
                    metaData.setParentFolderId(data.parentFolderId());
                    break;

                case QMailMessageKey::Sender:
                    metaData.setFrom(data.from());
                    break;

                case QMailMessageKey::Recipients:
                    metaData.setRecipients(data.recipients());
                    break;

                case QMailMessageKey::Subject:
                    metaData.setSubject(data.subject());
                    break;

                case QMailMessageKey::TimeStamp:
                    metaData.setDate(data.date());
                    break;

                case QMailMessageKey::ReceptionTimeStamp:
                    metaData.setReceivedDate(data.receivedDate());
                    break;

                case QMailMessageKey::Status:
                    metaData.setStatus(data.status());
                    break;

                case QMailMessageKey::ParentAccountId:
                    metaData.setParentAccountId(data.parentAccountId());
                    break;

                case QMailMessageKey::ServerUid:
                    metaData.setServerUid(data.serverUid());
                    break;

                case QMailMessageKey::Size:
                    metaData.setSize(data.size());
                    break;

                case QMailMessageKey::ContentType:
                    metaData.setContent(data.content());
                    break;

                case QMailMessageKey::PreviousParentFolderId:
                    metaData.setPreviousParentFolderId(data.previousParentFolderId());
                    break;

                case QMailMessageKey::ContentScheme:
                    metaData.setContentScheme(data.contentScheme());
                    break;

                case QMailMessageKey::ContentIdentifier:
                    metaData.setContentIdentifier(data.contentIdentifier());
                    break;

                case QMailMessageKey::InResponseTo:
                    metaData.setInResponseTo(data.inResponseTo());
                    break;

                case QMailMessageKey::ResponseType:
                    metaData.setResponseType(data.responseType());
                    break;

                case QMailMessageKey::ParentThreadId:
                    metaData.setParentThreadId(data.parentThreadId());
                    break;
                }
            }

            if (properties != allMessageProperties()) {
                // This message is not completely loaded
                metaData.setStatus(QMailMessage::UnloadedData, true);
            }

            metaData.setUnmodified();
            messageCache.insert(metaData);

        }
    }

    emit q_ptr->messagePropertyUpdated(ids, properties, data);
}

void QMailStorePrivate::messageStatusRemotelyChanged(const QMailMessageIdList& ids,
                                                     quint64 status, bool set)
{
    Q_ASSERT(!ids.contains(QMailMessageId()));

    foreach(const QMailMessageId& id, ids) {

        if(messageCache.contains(id)) {
            QMailMessageMetaData metaData = messageCache.lookup(id);
            metaData.setStatus(status, set);
            metaData.setUnmodified();
            messageCache.insert(metaData);
        }
    }

    emit q_ptr->messageStatusUpdated(ids, status, set);
}

void QMailStorePrivate::disconnectIpc()
{
    QMailStoreImplementation::disconnectIpc();

    ipcLastDbUpdated = QMail::lastDbUpdated();
}

void QMailStorePrivate::reconnectIpc()
{
    QMailStoreImplementation::reconnectIpc();

    // clear cache if needed
    const QDateTime& lastDbUpdated = QMail::lastDbUpdated();
    if (ipcLastDbUpdated != lastDbUpdated) {
        // Clear all caches
        accountCache.clear();
        folderCache.clear();
        messageCache.clear();
        uidCache.clear();
        threadCache.clear();

        ipcLastDbUpdated = lastDbUpdated;
    }
}
