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
#include <QSqlDatabase>
#include <QCache>
#include <QTimer>

//#define QMAILSTORE_LOG_SQL //define to enable SQL query logging
//#define QMAILSTORE_USE_RTTI //define if RTTI is available to assist debugging

#ifdef QMAILSTORE_USE_RTTI
#include <typeinfo>
#endif

class ProcessMutex;

struct ThreadUpdateData
{
    explicit ThreadUpdateData(const qint64 &changedMessagesCount = 0,
                     const qint64 &changedReadMessagesCount = 0,
                     const QString &newSubject = QString(),
                     const QString &newPreview = QString(),
                     const QString &newSenders = QString(),
                     const QMailTimeStamp &newLastDate = QMailTimeStamp(),
                     const QMailTimeStamp &newStartedDate = QMailTimeStamp(),
                     const qint64 &newStatus = 0 )
        : mMessagesCount(changedMessagesCount)
        , mReadMessagesCount(changedReadMessagesCount)
        , mNewSubject(newSubject)
        , mNewPreview(newPreview)
        , mNewSenders(newSenders)
        , mNewLastDate(newLastDate)
        , mNewStartedDate(newStartedDate)
        , mStatus(newStatus)
    {
    }

    explicit ThreadUpdateData(const qint64 &changedMessagesCount,
                     const qint64 &changedReadMessagesCount,
                     const qint64 &newStatus)
        : mMessagesCount(changedMessagesCount)
        , mReadMessagesCount(changedReadMessagesCount)
        , mNewSubject(QString())
        , mNewPreview(QString())
        , mNewSenders(QString())
        , mNewLastDate(QMailTimeStamp())
        , mNewStartedDate(QMailTimeStamp())
        , mStatus(newStatus)
    {
    }

    const qint64 mMessagesCount;
    const qint64 mReadMessagesCount;
    const QString mNewSubject;
    const QString mNewPreview;
    const QString mNewSenders;
    const QMailTimeStamp mNewLastDate;
    const QMailTimeStamp mNewStartedDate;
    const qint64 mStatus;
};

class QMailStorePrivate : public QMailStoreImplementation
{
    Q_OBJECT

public:
    typedef QMap<QMailMessageKey::Property, QString> MessagePropertyMap;
    typedef QList<QMailMessageKey::Property> MessagePropertyList;
    typedef QMap<QMailThreadKey::Property, QString> ThreadPropertyMap;
    typedef QList<QMailThreadKey::Property> ThreadPropertyList;

    class Transaction;
    struct ReadLock;
    class Key;

    struct ReadAccess {};
    struct WriteAccess {};

    QMailStorePrivate(QMailStore *parent);
    virtual ~QMailStorePrivate();

    virtual bool initStore();

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
                        QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders, QMailThreadIdList *deletedThreadIds, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                       QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailThreadIdList *deletedThreadIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                        QMailMessageIdList *deletedMessages, QMailThreadIdList* deletedThreadIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool removeThreads(const QMailThreadKey &key, QMailStore::MessageRemovalOption option,
                               QMailThreadIdList *deletedThreads, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIdList, QMailAccountIdList *modifiedAccountIds);


    virtual bool updateAccount(QMailAccount *account, QMailAccountConfiguration* config,
                       QMailAccountIdList *updatedAccountIds);

    virtual bool updateAccountConfiguration(QMailAccountConfiguration* config,
                                    QMailAccountIdList *updatedAccountIds);

    virtual bool updateFolder(QMailFolder* f,
                      QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateThread(QMailThread *t, QMailThreadIdList *updatedThreadIds);

    virtual bool updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &m,
                        QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads,  QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data,
                                QMailMessageIdList *updatedMessageIds, QMailThreadIdList *deletedThreads, QMailThreadIdList *modifiedThreads,  QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, quint64 messageStatus, bool set,
                                QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool ensureDurability();
    virtual bool shrinkMemory();

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

    QString buildOrderClause(const Key& key) const;

    QString buildWhereClause(const Key& key, bool nested = false, bool firstClause = true) const;
    QVariantList whereClauseValues(const Key& key) const;

    static QString expandValueList(const QVariantList& valueList);
    static QString expandValueList(int valueCount);

    static QString temporaryTableName(const QMailMessageKey::ArgumentType &arg);

    virtual QMap<QString, QString> messageCustomFields(const QMailMessageId &id);

    template<typename ValueType>
    static ValueType extractValue(const QVariant& var, const ValueType &defaultValue = ValueType());

    enum AttemptResult { Success = 0, Failure, DatabaseFailure };
public slots:
    void unloadDatabase();
    
private:
    friend class Transaction;
    friend struct ReadLock;

    static ProcessMutex& contentManagerMutex(void);

    ProcessMutex& databaseMutex(void) const;

    static const MessagePropertyMap& messagePropertyMap();
    static const MessagePropertyList& messagePropertyList();
    static const ThreadPropertyMap& threadPropertyMap();
    static const ThreadPropertyList& threadPropertyList();

    static const QMailMessageKey::Properties &updatableMessageProperties();
    static const QMailMessageKey::Properties &allMessageProperties();

    QString expandProperties(const QMailMessageKey::Properties& p, bool update = false) const;
    QString expandProperties(const QMailThreadKey::Properties& p, bool update = false) const;

    QString databaseIdentifier() const;

    bool ensureVersionInfo();
    qint64 tableVersion(const QString &name) const;
    bool setTableVersion(const QString &name, qint64 version);

    qint64 incrementTableVersion(const QString &name, qint64 current);
    bool upgradeTableVersion(const QString &name, qint64 current, qint64 final);

    bool upgradeTimeStampToUtc();

    bool fullThreadTableUpdate();

    bool createTable(const QString &name);

    typedef QPair<QString, qint64> TableInfo;
    bool setupTables(const QList<TableInfo> &tableList);

    struct FolderInfo {
        FolderInfo(quint64 id, QString const& name, quint64 status = 0)
            : _id(id), _name(name), _status(status)
        {}
        quint64 id() const { return _id; }
        QString name() const { return _name; }
        quint64 status() const { return _status; }
    private:
        quint64 _id;
        QString _name;
        quint64 _status;
    };

    bool setupFolders(const QList<FolderInfo> &folderList);

    bool purgeMissingAncestors();
    bool purgeObsoleteFiles();

    bool performMaintenanceTask(const QString &task, uint secondsFrequency, bool (QMailStorePrivate::*func)(void));

    bool performMaintenance();

    void createTemporaryTable(const QMailMessageKey::ArgumentType &arg, const QString &dataType) const;
    void destroyTemporaryTables(void);

    bool transaction(void);
    bool commit(void);
    void rollback(void);

    void setQueryError(const QSqlError&, const QString& description = QString(), const QString& statement = QString());
    void clearQueryError(void);

    QSqlQuery prepare(const QString& sql);
    bool execute(QSqlQuery& q, bool batch = false);
    int queryError(void) const;

    QSqlQuery performQuery(const QString& statement, bool batch, const QVariantList& bindValues, const QList<Key>& keys, const QPair<uint, uint> &constraint, const QString& descriptor);

    bool executeFile(QFile &file);

    QSqlQuery simpleQuery(const QString& statement, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const QString& descriptor);

    QSqlQuery simpleQuery(const QString& statement, const Key& key, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const Key& key, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QPair<uint, uint> &constraint, const QString& descriptor);

    QSqlQuery batchQuery(const QString& statement, const QVariantList& bindValues, const QString& descriptor);
    QSqlQuery batchQuery(const QString& statement, const QVariantList& bindValues, const Key& key, const QString& descriptor);
    QSqlQuery batchQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QString& descriptor);

    bool idValueExists(quint64 id, const QString& table);

    bool idExists(const QMailAccountId& id, const QString& table = QString());
    bool idExists(const QMailFolderId& id, const QString& table = QString());
    bool idExists(const QMailMessageId& id, const QString& table = QString());

    bool messageExists(const QString &serveruid, const QMailAccountId &id);

    bool checkPreconditions(const QMailFolder& folder, bool update = false);

    void preloadHeaderCache(const QMailMessageId& id) const;
    void preloadThreadCache(const QMailThreadId& id) const;

    QMailFolderIdList folderAncestorIds(const QMailFolderIdList& ids, bool inTransaction, AttemptResult *result) const;

    quint64 queryStatusMap(const QString &name, const QString &context, QMap<QString, quint64> &map) const;

    bool recalculateThreadsColumns(const QMailThreadIdList& modifiedThreads, QMailThreadIdList& deletedThreads);

    bool deleteMessages(const QMailMessageKey& key,
                        QMailStore::MessageRemovalOption option,
                        QMailMessageIdList& deletedMessageIds,
                        QMailThreadIdList& deletedThreadIds,
                        QStringList& expiredMailfiles,
                        QMailMessageIdList& updatedMessageIds,
                        QMailFolderIdList& modifiedFolders,
                        QMailThreadIdList& modifiedThreads,
                        QMailAccountIdList& modifiedAccounts);

    bool deleteFolders(const QMailFolderKey& key,
                       QMailStore::MessageRemovalOption option,
                       QMailFolderIdList& deletedFolderIds,
                       QMailMessageIdList& deletedMessageIds,
                       QMailThreadIdList& deletedThreadIds,
                       QStringList& expiredMailfiles,
                       QMailMessageIdList& updatedMessageIds,
                       QMailFolderIdList& modifiedFolderIds,
                       QMailThreadIdList& modifiedThreadIds,
                       QMailAccountIdList& modifiedAccountIds);

    bool deleteThreads(const QMailThreadKey& key,
                       QMailStore::MessageRemovalOption option,
                       QMailThreadIdList& deletedThreadIds,
                       QMailMessageIdList& deletedMessageIds,
                       QStringList& expiredMailfiles,
                       QMailMessageIdList& updatedMessageIds,
                       QMailFolderIdList& modifiedFolderIds,
                       QMailThreadIdList& modifiedThreadIds,
                       QMailAccountIdList& modifiedAccountIds);

    bool deleteAccounts(const QMailAccountKey& key,
                        QMailAccountIdList& deletedAccountIds,
                        QMailFolderIdList& deletedFolderIds,
                        QMailThreadIdList& deletedThreadIds,
                        QMailMessageIdList& deletedMessageIds,
                        QStringList& expiredMailfiles,
                        QMailMessageIdList& updatedMessageIds,
                        QMailFolderIdList& modifiedFolderIds,
                        QMailThreadIdList& modifiedThreadIds,
                        QMailAccountIdList& modifiedAccountIds);

    void removeExpiredData(const QMailMessageIdList& messageIds,
                           const QMailThreadIdList& threadIds,
                           const QStringList& mailfiles,
                           const QMailFolderIdList& folderIds = QMailFolderIdList(),
                           const QMailAccountIdList& accountIds = QMailAccountIdList());

    AttemptResult findPotentialPredecessorsBySubject(QMailMessageMetaData *metaData, const QString& baseSubject, bool* missingAncestor, QList<quint64>& potentialPredecessors);

    bool obsoleteContent(const QString& identifier);

    template<typename AccessType, typename FunctionType>
    bool repeatedly(FunctionType func, const QString &description, Transaction *t = Q_NULLPTR) const;

    quint64 threadId(const QMailMessageId &id);
    AttemptResult updateLatestInConversation(quint64 threadId, QMailMessageIdList *messagesUpdated, quint64 *updatedTo = Q_NULLPTR);
    AttemptResult updateLatestInConversation(const QSet<quint64> &threadIds, QMailMessageIdList *messagesUpdated);

    AttemptResult addCustomFields(quint64 id, const QMap<QString, QString> &fields, const QString &tableName);
    AttemptResult updateCustomFields(quint64 id, const QMap<QString, QString> &fields, const QString &tableName);
    AttemptResult customFields(quint64 id, QMap<QString, QString> *fields, const QString &tableName);

    AttemptResult attemptAddAccount(QMailAccount *account, QMailAccountConfiguration* config, 
                                    QMailAccountIdList *addedAccountIds, 
                                    Transaction &t, bool commitOnSuccess);

    AttemptResult attemptAddFolder(QMailFolder *folder, 
                                   QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                   Transaction &t, bool commitOnSuccess);

    AttemptResult attemptAddThread(QMailThread *thread, QMailThreadIdList *addedThreadIds, Transaction &t, bool commitOnSuccess);


    // hack to get around tr1's bind arg limit
    struct AttemptAddMessageOut {
        AttemptAddMessageOut( QMailMessageIdList * addedMessages
                            , QMailThreadIdList * addedThreads
                            , QMailMessageIdList * updatedMessages
                            , QMailThreadIdList * updatedThreads
                            , QMailFolderIdList * modifiedFolders
                            , QMailThreadIdList * modifiedThreads
                            , QMailAccountIdList * modifiedAccounts)
            : addedMessageIds(addedMessages)
            , addedThreadIds(addedThreads)
            , updatedMessageIds(updatedMessages)
            , updatedThreadIds(updatedThreads)
            , modifiedFolderIds(modifiedFolders)
            , modifiedThreadIds(modifiedThreads)
            , modifiedAccountIds(modifiedAccounts)
        {}

        QMailMessageIdList *addedMessageIds;
        QMailThreadIdList* addedThreadIds;
        QMailMessageIdList *updatedMessageIds;
        QMailThreadIdList *updatedThreadIds;
        QMailFolderIdList *modifiedFolderIds;
        QMailThreadIdList *modifiedThreadIds;
        QMailAccountIdList *modifiedAccountIds;
    };


    AttemptResult attemptAddMessage(QMailMessage *message, const QString &identifier, const QStringList &references, AttemptAddMessageOut *out,
                                    Transaction &t, bool commitOnSuccess);

    AttemptResult attemptAddMessage(QMailMessageMetaData *metaData, const QString &identifier, const QStringList &references, AttemptAddMessageOut *out,
                                    Transaction &t, bool commitOnSuccess);


    struct AttemptRemoveAccountOut {
        AttemptRemoveAccountOut( QMailAccountIdList *deletedAccounts
                               , QMailFolderIdList *deletedFolders
                               , QMailThreadIdList *deletedThreads
                               , QMailMessageIdList *deletedMessages
                               , QMailMessageIdList *updatedMessages
                               , QMailFolderIdList *modifiedFolders
                               , QMailThreadIdList *modifiedThreads
                               , QMailAccountIdList *modifiedAccounts)
            : deletedAccountIds(deletedAccounts)
            , deletedFolderIds(deletedFolders)
            , deletedThreadIds(deletedThreads)
            , deletedMessageIds(deletedMessages)
            , updatedMessageIds(updatedMessages)
            , modifiedFolderIds(modifiedFolders)
            , modifiedThreadIds(modifiedThreads)
            , modifiedAccountIds(modifiedAccounts)
        {}

        QMailAccountIdList *deletedAccountIds;
        QMailFolderIdList *deletedFolderIds;
        QMailThreadIdList *deletedThreadIds;
        QMailMessageIdList *deletedMessageIds;
        QMailMessageIdList *updatedMessageIds;
        QMailFolderIdList *modifiedFolderIds;
        QMailThreadIdList *modifiedThreadIds;
        QMailAccountIdList *modifiedAccountIds;
    };


    AttemptResult attemptRemoveAccounts(const QMailAccountKey &key, 
                                        AttemptRemoveAccountOut *out,
                                        Transaction &t, bool commitOnSuccess);

    // a hack to get around bind max arg limitation
    struct AttemptRemoveFoldersOut {

        AttemptRemoveFoldersOut(QMailFolderIdList *deletedFolders
                              , QMailMessageIdList *deletedMessages
                              , QMailThreadIdList *deletedThreads
                              , QMailMessageIdList *updatedMessages
                              , QMailFolderIdList *modifiedFolders
                              , QMailThreadIdList *modifiedThreads
                              , QMailAccountIdList *modifiedAccounts)
            : deletedFolderIds(deletedFolders)
            , deletedMessageIds(deletedMessages)
            , deletedThreadIds(deletedThreads)
            , updatedMessageIds(updatedMessages)
            , modifiedFolderIds(modifiedFolders)
            , modifiedThreadIds(modifiedThreads)
            , modifiedAccountIds(modifiedAccounts)
        {}

        QMailFolderIdList *deletedFolderIds;
        QMailMessageIdList *deletedMessageIds;
        QMailThreadIdList *deletedThreadIds;
        QMailMessageIdList *updatedMessageIds;
        QMailFolderIdList *modifiedFolderIds;
        QMailThreadIdList *modifiedThreadIds;
        QMailAccountIdList *modifiedAccountIds;
    };


    AttemptResult attemptRemoveFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option, 
                                       AttemptRemoveFoldersOut *out,
                                       Transaction &t, bool commitOnSuccess);


    // hack to get around bind max arg limitation
    struct AttemptRemoveThreadsOut {
        AttemptRemoveThreadsOut( QMailThreadIdList *deletedThreads
                               , QMailMessageIdList *deletedMessages
                               , QMailMessageIdList *updatedMessages
                               , QMailFolderIdList *modifiedFolders
                               , QMailThreadIdList *modifiedThreads
                               , QMailAccountIdList *modifiedAccount)
            : deletedThreadIds(deletedThreads)
            , deletedMessageIds(deletedMessages)
            , updatedMessageIds(updatedMessages)
            , modifiedFolderIds(modifiedFolders)
            , modifiedThreadIds(modifiedThreads)
            , modifiedAccountIds(modifiedAccount)
        {}

        QMailThreadIdList *deletedThreadIds;
        QMailMessageIdList *deletedMessageIds;
        QMailMessageIdList *updatedMessageIds;
        QMailFolderIdList *modifiedFolderIds;
        QMailThreadIdList *modifiedThreadIds;
        QMailAccountIdList *modifiedAccountIds;
    };

    AttemptResult attemptRemoveThreads(const QMailThreadKey &key, QMailStore::MessageRemovalOption option,
                                       AttemptRemoveThreadsOut *out,
                                       Transaction &t, bool commitOnSuccess);

    AttemptResult attemptRemoveMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                                        QMailMessageIdList *deletedMessages, QMailThreadIdList* deletedThreadIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailThreadIdList *modifiedThreadIds, QMailAccountIdList *modifiedAccountIds,
                                        Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateAccount(QMailAccount *account, QMailAccountConfiguration *config,
                                       QMailAccountIdList *updatedAccountIds,
                                       Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateAccountConfiguration(QMailAccountConfiguration *config, 
                                                    QMailAccountIdList *updatedAccountIds,
                                                    Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateFolder(QMailFolder *folder,
                                      QMailFolderIdList *updatedFolderIds, QMailAccountIdList *updatedAccounts,
                                      Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateThread(QMailThread *thread,
                                      QMailThreadIdList *updatedThreadIds,
                                      Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateMessage(QMailMessageMetaData *metaData, QMailMessage *mail,
                                       QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads, QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, QMap<QString, QStringList> *deleteLaterContent,
                                       Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &props, const QMailMessageMetaData &data,
                                                QMailMessageIdList *updatedMessageIds, QMailThreadIdList* deletedThreadIds, QMailThreadIdList *modifiedThreads, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateMessagesStatus(const QMailMessageKey &key, quint64 status, bool set,
                                              QMailMessageIdList *updatedMessageIds, QMailThreadIdList *modifiedThreads, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                              Transaction &t, bool commitOnSuccess);

    AttemptResult attemptPurgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids,
                                                    Transaction &t, bool commitOnSuccess);

    AttemptResult attemptEnsureDurability(Transaction &t, bool commitOnSuccess);

    AttemptResult attemptCountAccounts(const QMailAccountKey &key, int *result, 
                                       ReadLock &);

    AttemptResult attemptCountFolders(const QMailFolderKey &key, int *result, 
                                      ReadLock &);

    AttemptResult attemptCountMessages(const QMailMessageKey &key, 
                                       int *result, 
                                       ReadLock &);

    AttemptResult attemptCountThreads(const QMailThreadKey &key,
                                       int *result,
                                       ReadLock &);

    AttemptResult attemptSizeOfMessages(const QMailMessageKey &key, 
                                        int *result, 
                                        ReadLock &);

    AttemptResult attemptQueryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, uint limit, uint offset,
                                       QMailAccountIdList *ids, 
                                       ReadLock &);

    AttemptResult attemptQueryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, uint limit, uint offset,
                                      QMailFolderIdList *ids, 
                                      ReadLock &);

    AttemptResult attemptQueryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey, uint limit, uint offset,
                                       QMailMessageIdList *ids, 
                                       ReadLock &);

    AttemptResult attemptQueryThreads(const QMailThreadKey &key, const QMailThreadSortKey &sortKey, uint limit, uint offset,
                                      QMailThreadIdList *ids,
                                      ReadLock &);

    AttemptResult attemptAccount(const QMailAccountId &id, 
                                 QMailAccount *result, 
                                 ReadLock &);

    AttemptResult attemptAccountConfiguration(const QMailAccountId &id, 
                                              QMailAccountConfiguration *result, 
                                              ReadLock &);

    AttemptResult attemptFolder(const QMailFolderId &id, 
                                QMailFolder *result, 
                                ReadLock &);

    AttemptResult attemptMessage(const QMailMessageId &id, 
                                 QMailMessage *result, 
                                 ReadLock &);

    AttemptResult attemptMessage(const QString &uid, const QMailAccountId &accountId, 
                                 QMailMessage *result, 
                                 ReadLock &);

    AttemptResult attemptMessageMetaData(const QMailMessageId &id, 
                                         QMailMessageMetaData *result, 
                                         ReadLock &);

    AttemptResult attemptMessageMetaData(const QString &uid, const QMailAccountId &accountId, 
                                         QMailMessageMetaData *result, 
                                         ReadLock &);

    AttemptResult attemptMessagesMetaData(const QMailMessageKey& key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option, 
                                          QMailMessageMetaDataList *result, 
                                          ReadLock &);

    AttemptResult attemptThread(const QMailThreadId &id,
                                QMailThread *result,
                                ReadLock &);

    AttemptResult attemptThreads(const QMailThreadKey& key,
                                 QMailStore::ReturnOption option,
                                 QMailThreadList *result,
                                 ReadLock &);

    AttemptResult attemptMessageRemovalRecords(const QMailAccountId &accountId, const QMailFolderId &parentFolderId,
                                               QMailMessageRemovalRecordList *result,
                                               ReadLock &);

    AttemptResult attemptMessageFolderIds(const QMailMessageKey &key, 
                                          QMailFolderIdList *result, 
                                          ReadLock &);

    AttemptResult attemptFolderAccountIds(const QMailFolderKey &key, 
                                          QMailAccountIdList *result, 
                                          ReadLock &);

    AttemptResult attemptFolderAncestorIds(const QMailFolderIdList &ids, 
                                           QMailFolderIdList *result, 
                                           ReadLock &);

    AttemptResult attemptStatusBit(const QString &name, const QString &context, 
                                   int *result, 
                                   ReadLock &);

    AttemptResult attemptRegisterStatusBit(const QString &name, const QString &context, int maximum, bool check, quint64 *number,
                                           Transaction &t, bool commitOnSuccess);

    AttemptResult attemptMessageId(const QString &uid, const QMailAccountId &accountId, 
                                   quint64 *result, 
                                   ReadLock &);

    AttemptResult affectedByMessageIds(const QMailMessageIdList &messages, QMailFolderIdList *folderIds, QMailAccountIdList *accountIds) const;

    AttemptResult affectedByFolderIds(const QMailFolderIdList &folders, QMailFolderIdList *folderIds, QMailAccountIdList *accountIds) const;

    AttemptResult messagePredecessor(QMailMessageMetaData *metaData, const QStringList &references, const QString &baseSubject, bool sameSubject, QStringList *missingReferences, bool *missingAncestor);

    AttemptResult identifyAncestors(const QMailMessageId &predecessorId, const QMailMessageIdList &childIds, QMailMessageIdList *ancestorIds);

    AttemptResult resolveMissingMessages(const QString &identifier, const QMailMessageId &predecessorId, const QString &baseSubject, const QMailMessageMetaData &message, QMailMessageIdList *updatedMessageIds);

    AttemptResult registerSubject(const QString &baseSubject, quint64 messageId, const QMailMessageId &predecessorId, bool missingAncestor);

    QMailAccount extractAccount(const QSqlRecord& r);
    QMailThread extractThread(const QSqlRecord &r);
    QMailFolder extractFolder(const QSqlRecord& r);
    QMailMessageMetaData extractMessageMetaData(const QSqlRecord& r, QMailMessageKey::Properties recordProperties, const QMailMessageKey::Properties& properties = allMessageProperties());
    QMailMessageMetaData extractMessageMetaData(const QSqlRecord& r, const QMap<QString, QString> &customFields, const QMailMessageKey::Properties& properties = allMessageProperties());
    QMailMessage extractMessage(const QSqlRecord& r, const QMap<QString, QString> &customFields, const QMailMessageKey::Properties& properties = allMessageProperties());
    QMailMessageRemovalRecord extractMessageRemovalRecord(const QSqlRecord& r);

    virtual void emitIpcNotification(QMailStoreImplementation::AccountUpdateSignal signal, const QMailAccountIdList &ids);
    virtual void emitIpcNotification(QMailStoreImplementation::FolderUpdateSignal signal, const QMailFolderIdList &ids);
    virtual void emitIpcNotification(QMailStoreImplementation::ThreadUpdateSignal signal, const QMailThreadIdList &ids);
    virtual void emitIpcNotification(QMailStoreImplementation::MessageUpdateSignal signal, const QMailMessageIdList &ids);
    virtual void emitIpcNotification(QMailStoreImplementation::MessageDataPreCacheSignal signal, const QMailMessageMetaDataList &data);
    virtual void emitIpcNotification(const QMailMessageIdList& ids,  const QMailMessageKey::Properties& properties,
                                     const QMailMessageMetaData& data);
    virtual void emitIpcNotification(const QMailMessageIdList& ids, quint64 status, bool set);

    static const int messageCacheSize = 100;
    static const int threadCacheSize = 300;
    static const int uidCacheSize = 500;
    static const int folderCacheSize = 100;
    static const int accountCacheSize = 10;
    static const int lookAhead = 5;

    static QString parseSql(QTextStream& ts);

    static QVariantList messageValues(const QMailMessageKey::Properties& properties, const QMailMessageMetaData& data);
    static QVariantList threadValues(const QMailThreadKey::Properties& properties, const QMailThread& thread);
    static void updateMessageValues(const QMailMessageKey::Properties& properties, const QVariantList& values, const QMap<QString, QString>& customFields, QMailMessageMetaData& metaData);
    AttemptResult updateThreadsValues(const QMailThreadIdList& threadsToDelete,
                                      const QMailThreadIdList& modifiedThreadsIds = QMailThreadIdList(),
                                      const ThreadUpdateData& updateData = ThreadUpdateData());

    static const QString &defaultContentScheme();
    static const QString &messagesBodyPath();
    static QString messageFilePath(const QString &fileName);

    static void extractMessageMetaData(const QSqlRecord& r, QMailMessageKey::Properties recordProperties, const QMailMessageKey::Properties& properties, QMailMessageMetaData* metaData);

private:
    Q_DECLARE_PUBLIC (QMailStore)
    QMailStore * const q_ptr;

    template <typename T, typename KeyType> 
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
        QCache<KeyType,T> mCache;
    };

    template <typename T, typename ID> 
    class IdCache : public Cache<T, quint64>
    {
    public:
        IdCache(unsigned int size = 10) : Cache<T, quint64>(size) {}

        T lookup(const ID& id) const;
        void insert(const T& item);
        bool contains(const ID& id) const;
        void remove(const ID& id);
    };

    QSqlDatabase *database() const;
    mutable QSqlDatabase *databaseptr;
    mutable QTimer databaseUnloadTimer;

    mutable QMailMessageIdList lastQueryMessageResult;
    mutable QMailThreadIdList lastQueryThreadResult;

    mutable IdCache<QMailMessageMetaData, QMailMessageId> messageCache;
    mutable Cache<QMailMessageId, QPair<QMailAccountId, QString> > uidCache;
    mutable IdCache<QMailFolder, QMailFolderId> folderCache;
    mutable IdCache<QMailAccount, QMailAccountId> accountCache;
    mutable IdCache<QMailThread, QMailThreadId> threadCache;

    mutable QList<QPair<const QMailMessageKey::ArgumentType*, QString> > requiredTableKeys;
    mutable QList<const QMailMessageKey::ArgumentType*> temporaryTableKeys;
    QList<const QMailMessageKey::ArgumentType*> expiredTableKeys;

    bool inTransaction;
    mutable int lastQueryError;

    ProcessMutex *mutex;

    static ProcessMutex *contentMutex;

    int globalLocks;
};

template <typename ValueType>
ValueType QMailStorePrivate::extractValue(const QVariant &var, const ValueType &defaultValue)
{
    if (!var.canConvert<ValueType>()) {
        qWarning() << "QMailStorePrivate::extractValue - Cannot convert variant to:"
#ifdef QMAILSTORE_USE_RTTI
                   << typeid(ValueType).name();
#else
                   << "requested type";
#endif
        return defaultValue;
    }

    return var.value<ValueType>();
}


template <typename T, typename KeyType> 
QMailStorePrivate::Cache<T, KeyType>::Cache(unsigned int cacheSize)
    : mCache(cacheSize)
{
}

template <typename T, typename KeyType> 
QMailStorePrivate::Cache<T, KeyType>::~Cache()
{
}

template <typename T, typename KeyType> 
T QMailStorePrivate::Cache<T, KeyType>::lookup(const KeyType& key) const
{
    if (T* cachedItem = mCache.object(key))
        return *cachedItem;

    return T();
}

template <typename T, typename KeyType> 
void QMailStorePrivate::Cache<T, KeyType>::insert(const KeyType& key, const T& item)
{
    mCache.insert(key,new T(item));
}

template <typename T, typename KeyType> 
bool QMailStorePrivate::Cache<T, KeyType>::contains(const KeyType& key) const
{
    return mCache.contains(key);
}

template <typename T, typename KeyType> 
void QMailStorePrivate::Cache<T, KeyType>::remove(const KeyType& key)
{
    mCache.remove(key);
}

template <typename T, typename KeyType> 
void QMailStorePrivate::Cache<T, KeyType>::clear()
{
    mCache.clear();
}


template <typename T, typename ID> 
T QMailStorePrivate::IdCache<T, ID>::lookup(const ID& id) const
{
    if (id.isValid()) {
        return Cache<T, quint64>::lookup(id.toULongLong());
    }

    return T();
}

template <typename T, typename ID> 
void QMailStorePrivate::IdCache<T, ID>::insert(const T& item)
{
    if (item.id().isValid()) {
        Cache<T, quint64>::insert(item.id().toULongLong(), item);
    }
}

template <typename T, typename ID> 
bool QMailStorePrivate::IdCache<T, ID>::contains(const ID& id) const
{
    return Cache<T, quint64>::contains(id.toULongLong());
}

template <typename T, typename ID> 
void QMailStorePrivate::IdCache<T, ID>::remove(const ID& id)
{
    Cache<T, quint64>::remove(id.toULongLong());
}

#endif
