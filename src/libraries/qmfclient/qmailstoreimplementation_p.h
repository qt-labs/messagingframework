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
#include "qmailstorenotifier_p.h"

#include <QList>
#include <QPair>
#include <QString>

class QMF_EXPORT QMailStoreImplementation : public QMailStoreNotifier
{
public:
    QMailStoreImplementation(QMailStore* parent);
    virtual ~QMailStoreImplementation();

    void initialize();
    static QMailStore::InitializationState initializationState();

    virtual QMailStore::ErrorCode lastError() const = 0;
    virtual void setLastError(QMailStore::ErrorCode code) const = 0;

    virtual bool ensureDurability() = 0;

    virtual void lock() = 0;
    virtual void unlock() = 0;

    virtual void clearContent() = 0;

    virtual bool addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                            QMailAccountIdList *addedAccountIds) = 0;

    virtual bool addFolder(QMailFolder *f,
                           QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool addMessages(const QList<QMailMessage *> &m,
                             QMailMessageIdList *addedMessageIds, QMailThreadIdList *addedThreadIds, QMailMessageIdList *updatedMessageIds, QMailThreadIdList *updatedThreadIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds) = 0;

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

protected:
    static QMailStore::InitializationState initState;

private:
    virtual bool initStore() = 0;

    QMailStore* q;
};

class QMF_EXPORT QMailStoreNullImplementation : public QMailStoreImplementation
{
public:
    QMailStoreNullImplementation(QMailStore* parent);

    QMailStore::ErrorCode lastError() const override;
    void setLastError(QMailStore::ErrorCode code) const override;

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
                        QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds,
                        QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds) override;

    bool removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                       QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages,
                       QMailThreadIdList *deletedThreadIds, QMailMessageIdList *updatedMessages,
                       QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds,
                       QMailAccountIdList *modifiedAccountIds) override;

    bool removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                        QMailMessageIdList *deletedMessages, QMailThreadIdList* deletedThreadIds,
                        QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds,
                        QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds) override;

    bool removeThreads(const QMailThreadKey &key, QMailStore::MessageRemovalOption option,
                       QMailThreadIdList *deletedThreads,
                       QMailMessageIdList *deletedMessageIds,
                       QMailMessageIdList *updatedMessageIds,
                       QMailFolderIdList *modifiedFolderIds,
                       QMailThreadIdList *modifiedThreadIdList,
                       QMailAccountIdList *modifiedAccountIds) override;

    bool updateAccount(QMailAccount *account, QMailAccountConfiguration* config,
                               QMailAccountIdList *updatedAccountIds) override;

    bool updateAccountConfiguration(QMailAccountConfiguration* config,
                                    QMailAccountIdList *updatedAccountIds) override;

    bool updateFolder(QMailFolder* f,
                      QMailFolderIdList *updatedFolderIds,
                      QMailAccountIdList *modifiedAccountIds) override;

    bool updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &m,
                        QMailMessageIdList *updatedMessageIds,
                        QMailThreadIdList *modifiedThreads,
                        QMailMessageIdList *modifiedMessageIds,
                        QMailFolderIdList *modifiedFolderIds,
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

    bool updateThread(QMailThread *t, QMailThreadIdList *updatedThreadIds) override;

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
    QMailMessageMetaData messageMetaData(const QString &uid,
                                         const QMailAccountId &accountId) const override;
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

    void accountsRemotelyChanged(QMailStore::ChangeType,
                                 const QMailAccountIdList&) override {}
    void messageRemovalRecordsRemotelyChanged(QMailStore::ChangeType,
                                              const QMailAccountIdList&) override {}
    void remoteTransmissionInProgress(const QMailAccountIdList&) override {}
    void remoteRetrievalInProgress(const QMailAccountIdList&) override {}
    void messagesRemotelyChanged(QMailStore::ChangeType,
                                 const QMailMessageIdList&) override {}
    void messageMetaDataRemotelyChanged(QMailStore::ChangeType,
                                        const QMailMessageMetaDataList&) override {}
    void messagePropertiesRemotelyChanged(const QMailMessageIdList&,
                                          QMailMessageKey::Properties,
                                          const QMailMessageMetaData&) override {}
    void messageStatusRemotelyChanged(const QMailMessageIdList&,
                                      quint64, bool) override {}
    void threadsRemotelyChanged(QMailStore::ChangeType,
                                const QMailThreadIdList&) override {}
    void foldersRemotelyChanged(QMailStore::ChangeType,
                                const QMailFolderIdList&) override {}

private:
    bool initStore() override;
};

#endif

