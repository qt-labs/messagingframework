/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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

//#define QMAILSTORE_LOG_SQL //define to enable SQL query logging
//#define QMAILSTORE_USE_RTTI //define if RTTI is available to assist debugging

#ifdef QMAILSTORE_USE_RTTI
#include <typeinfo>
#endif

class ProcessMutex;
class ProcessReadLock;


class QMailStorePrivate : public QMailStoreImplementation
{
    Q_OBJECT

public:
    typedef QMap<QMailMessageKey::Property, QString> MessagePropertyMap;
    typedef QList<QMailMessageKey::Property> MessagePropertyList;

    class Transaction;
    class ReadLock;
    class Key;

    struct ReadAccess {};
    struct WriteAccess {};

    QMailStorePrivate(QMailStore* parent);
    ~QMailStorePrivate();

    virtual bool initStore();
    void clearContent();

    bool addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                    QMailAccountIdList *addedAccountIds);

    bool addFolder(QMailFolder *f,
                   QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool addMessage(QMailMessage *m,
                    QMailMessageIdList *addedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool addMessage(QMailMessageMetaData *m,
                    QMailMessageIdList *addedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool removeAccounts(const QMailAccountKey &key,
                        QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages);

    bool removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                       QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailAccountIdList *modifiedAccounts);

    bool removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                        QMailMessageIdList *deletedMessages, QMailAccountIdList *modifiedAccounts, QMailFolderIdList *modifiedFolders);

    bool updateAccount(QMailAccount *account, QMailAccountConfiguration* config,
                       QMailAccountIdList *updatedAccountIds);

    bool updateAccountConfiguration(QMailAccountConfiguration* config,
                                    QMailAccountIdList *updatedAccountIds);

    bool updateFolder(QMailFolder* f,
                      QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool updateMessage(QMailMessageMetaData *metaData, QMailMessage *mail,
                       QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, bool *modifiedContent);

    bool updateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data,
                                QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool updateMessagesMetaData(const QMailMessageKey &key, quint64 messageStatus, bool set,
                                QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool restoreToPreviousFolder(const QMailMessageKey &key,
                                 QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool purgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids);

    int countAccounts(const QMailAccountKey &key) const;
    int countFolders(const QMailFolderKey &key) const;
    int countMessages(const QMailMessageKey &key) const;

    int sizeOfMessages(const QMailMessageKey &key) const;

    QMailAccountIdList queryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey) const;
    QMailFolderIdList queryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey) const;
    QMailMessageIdList queryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey) const;

    QMailAccount account(const QMailAccountId &id) const;
    QMailAccountConfiguration accountConfiguration(const QMailAccountId &id) const;

    QMailFolder folder(const QMailFolderId &id) const;

    QMailMessage message(const QMailMessageId &id) const;
    QMailMessage message(const QString &uid, const QMailAccountId &accountId) const;

    QMailMessageMetaData messageMetaData(const QMailMessageId &id) const;
    QMailMessageMetaData messageMetaData(const QString &uid, const QMailAccountId &accountId) const;
    QMailMessageMetaDataList messagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option) const;

    QMailMessageRemovalRecordList messageRemovalRecords(const QMailAccountId &parentAccountId, const QMailFolderId &parentFolderId) const;

    bool registerAccountStatusFlag(const QString &name);
    quint64 accountStatusMask(const QString &name) const;

    bool registerFolderStatusFlag(const QString &name);
    quint64 folderStatusMask(const QString &name) const;

    bool registerMessageStatusFlag(const QString &name);
    quint64 messageStatusMask(const QString &name) const;

    QString buildOrderClause(const Key& key) const;

    QString buildWhereClause(const Key& key, bool nested = false, bool firstClause = true) const;
    QVariantList whereClauseValues(const Key& key) const;

    static QString expandValueList(const QVariantList& valueList);
    static QString expandValueList(int valueCount);

    static QString temporaryTableName(const QMailMessageKey &key);

    template<typename ValueType>
    static ValueType extractValue(const QVariant& var, const ValueType &defaultValue = ValueType());

private:
    friend class Transaction;
    friend class ReadLock;

    enum AttemptResult { Success = 0, Failure, DatabaseFailure };
    
    static ProcessMutex& contentManagerMutex(void);

    ProcessMutex& databaseMutex(void) const;
    ProcessReadLock& databaseReadLock(void) const;

    static const MessagePropertyMap& messagePropertyMap();
    static const MessagePropertyList& messagePropertyList();

    static const QMailMessageKey::Properties &updatableMessageProperties();
    static const QMailMessageKey::Properties &allMessageProperties();

    bool containsProperty(const QMailMessageKey::Property& p, const QMailMessageKey& key) const;
    bool containsProperty(const QMailMessageSortKey::Property& p, const QMailMessageSortKey& sortkey) const;

    QString expandProperties(const QMailMessageKey::Properties& p, bool update = false) const;

    int databaseIdentifier(int n) const;

    bool ensureVersionInfo();
    qint64 tableVersion(const QString &name) const;
    bool setTableVersion(const QString &name, qint64 version);

    qint64 incrementTableVersion(const QString &name, qint64 current);
    bool upgradeTableVersion(const QString &name, qint64 current, qint64 final);

    bool createTable(const QString &name);

    typedef QPair<QString, qint64> TableInfo;
    bool setupTables(const QList<TableInfo> &tableList);

    typedef QPair<quint64, QString> FolderInfo;
    bool setupFolders(const QList<FolderInfo> &folderList);

    void createTemporaryTable(const QMailMessageKey &key) const;
    void destroyTemporaryTables(void);

    bool transaction(void);
    bool commit(void);
    void rollback(void);

    void setQueryError(const QSqlError&, const QString& description = QString(), const QString& statement = QString());
    void clearQueryError(void);

    QSqlQuery prepare(const QString& sql);
    bool execute(QSqlQuery& q, bool batch = false);
    int queryError(void) const;

    QSqlQuery performQuery(const QString& statement, bool batch, const QVariantList& bindValues, const QList<Key>& keys, const QString& descriptor);

    bool executeFile(QFile &file);

    QSqlQuery simpleQuery(const QString& statement, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const QString& descriptor);

    QSqlQuery simpleQuery(const QString& statement, const Key& key, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const Key& key, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QString& descriptor);

    QSqlQuery batchQuery(const QString& statement, const QVariantList& bindValues, const QString& descriptor);
    QSqlQuery batchQuery(const QString& statement, const QVariantList& bindValues, const Key& key, const QString& descriptor);
    QSqlQuery batchQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QString& descriptor);

    bool idValueExists(quint64 id, const QString& table);

    bool idExists(const QMailAccountId& id, const QString& table = QString());
    bool idExists(const QMailFolderId& id, const QString& table = QString());
    bool idExists(const QMailMessageId& id, const QString& table = QString());

    bool checkPreconditions(const QMailFolder& folder, bool update = false);

    void preloadHeaderCache(const QMailMessageId& id) const;

    QMailFolderIdList folderAncestorIds(const QMailFolderIdList& ids, bool inTransaction, AttemptResult *result) const;

    quint64 queryStatusMap(const QString &name, const QString &context, QMap<QString, quint64> &map) const;

    bool deleteMessages(const QMailMessageKey& key,
                        QMailStore::MessageRemovalOption option,
                        QMailMessageIdList& deletedMessageIds,
                        QStringList& expiredMailfiles,
                        QMailAccountIdList& modifiedAccounts,
                        QMailFolderIdList& modifiedFolders);

    bool deleteFolders(const QMailFolderKey& key,
                       QMailStore::MessageRemovalOption option,
                       QMailFolderIdList& deletedFolders,
                       QMailMessageIdList& deletedMessageIds,
                       QStringList& expiredMailfiles,
                       QMailAccountIdList& modifiedAccounts);

    bool deleteAccounts(const QMailAccountKey& key,
                        QMailAccountIdList& deletedAccounts,
                        QMailFolderIdList& deletedFolders,
                        QMailMessageIdList& deletedMessageIds,
                        QStringList& expiredMailfile);

    void removeExpiredData(const QMailMessageIdList& messageIds,
                           const QStringList& mailfiles,
                           const QMailFolderIdList& folderIds = QMailFolderIdList(),
                           const QMailAccountIdList& accountIds = QMailAccountIdList());

    template<typename AccessType, typename FunctionType>
    bool repeatedly(FunctionType func, const QString &description) const;

    AttemptResult addCustomFields(quint64 id, const QMap<QString, QString> &fields, const QString &tableName);
    AttemptResult updateCustomFields(quint64 id, const QMap<QString, QString> &fields, const QString &tableName);
    AttemptResult customFields(quint64 id, QMap<QString, QString> *fields, const QString &tableName);

    AttemptResult attemptAddAccount(QMailAccount *account, QMailAccountConfiguration* config, 
                                    QMailAccountIdList *addedAccountIds, 
                                    Transaction &t);

    AttemptResult attemptAddFolder(QMailFolder *folder, 
                                   QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                   Transaction &t);

    AttemptResult attemptAddMessage(QMailMessageMetaData *metaData,
                                    QMailMessageIdList *addedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, 
                                    Transaction &t);

    AttemptResult attemptRemoveAccounts(const QMailAccountKey &key, 
                                        QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages,
                                        Transaction &t);

    AttemptResult attemptRemoveFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option, 
                                       QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailAccountIdList *modifiedAccounts,
                                       Transaction &t);

    AttemptResult attemptRemoveMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option, 
                                        QMailMessageIdList *deletedMessages, QMailAccountIdList *modifiedAccounts, QMailFolderIdList *modifiedFolders,
                                        Transaction &t);

    AttemptResult attemptUpdateAccount(QMailAccount *account, QMailAccountConfiguration *config, 
                                       QMailAccountIdList *updatedAccountIds,
                                       Transaction &t);

    AttemptResult attemptUpdateAccountConfiguration(QMailAccountConfiguration *config, 
                                                    QMailAccountIdList *updatedAccountIds,
                                                    Transaction &t);

    AttemptResult attemptUpdateFolder(QMailFolder *folder, 
                                      QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                      Transaction &t);

    AttemptResult attemptUpdateMessage(QMailMessageMetaData *metaData, QMailMessage *mail, 
                                       QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, bool *modifiedContent,
                                       Transaction &t);

    AttemptResult affectedByMessageIds(const QMailMessageIdList &messages, QMailFolderIdList *folderIds, QMailAccountIdList *accountIds) const;

    AttemptResult affectedByFolderIds(const QMailFolderIdList &folders, QMailFolderIdList *folderIds, QMailAccountIdList *accountIds) const;

    AttemptResult attemptUpdateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &props, const QMailMessageMetaData &data, 
                                                QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                Transaction &t); 

    AttemptResult attemptUpdateMessagesStatus(const QMailMessageKey &key, quint64 status, bool set, 
                                              QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, 
                                              Transaction &t);

    AttemptResult attemptRestoreToPreviousFolder(const QMailMessageKey &key, 
                                                 QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, 
                                                 Transaction &t);

    AttemptResult attemptPurgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids,
                                                    Transaction &t);

    AttemptResult attemptCountAccounts(const QMailAccountKey &key, int *result, 
                                       ReadLock &);

    AttemptResult attemptCountFolders(const QMailFolderKey &key, int *result, 
                                      ReadLock &);

    AttemptResult attemptCountMessages(const QMailMessageKey &key, 
                                       int *result, 
                                       ReadLock &);

    AttemptResult attemptSizeOfMessages(const QMailMessageKey &key, 
                                        int *result, 
                                        ReadLock &);

    AttemptResult attemptQueryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, 
                                       QMailAccountIdList *ids, 
                                       ReadLock &);

    AttemptResult attemptQueryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, 
                                      QMailFolderIdList *ids, 
                                      ReadLock &);

    AttemptResult attemptQueryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey,
                                       QMailMessageIdList *ids, 
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

    AttemptResult attemptMessagesMetaData(const QMailMessageKey& key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option, 
                                          QMailMessageMetaDataList *result, 
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

    AttemptResult attemptRegisterStatusBit(const QString &name, const QString &context, int maximum, 
                                           Transaction &t);

    QMailAccount extractAccount(const QSqlRecord& r);
    QMailFolder extractFolder(const QSqlRecord& r);
    QMailMessageMetaData extractMessageMetaData(const QSqlRecord& r, QMailMessageKey::Properties recordProperties, const QMailMessageKey::Properties& properties = allMessageProperties());
    QMailMessage extractMessage(const QSqlRecord& r, const QMap<QString, QString> &customFields, const QMailMessageKey::Properties& properties = allMessageProperties());
    QMailMessageRemovalRecord extractMessageRemovalRecord(const QSqlRecord& r);

    virtual void emitIpcNotification(QMailStoreImplementation::AccountUpdateSignal signal, const QMailAccountIdList &ids);
    virtual void emitIpcNotification(QMailStoreImplementation::FolderUpdateSignal signal, const QMailFolderIdList &ids);
    virtual void emitIpcNotification(QMailStoreImplementation::MessageUpdateSignal signal, const QMailMessageIdList &ids);

    static const int headerCacheSize = 100;
    static const int folderCacheSize = 10;
    static const int accountCacheSize = 10;
    static const int lookAhead = 5;

    static QString parseSql(QTextStream& ts);

    static QVariantList messageValues(const QMailMessageKey::Properties& properties, const QMailMessageMetaData& data);
    static void updateMessageValues(const QMailMessageKey::Properties& properties, const QVariantList& values, const QMap<QString, QString>& customFields, QMailMessageMetaData& metaData);

    static const QString &defaultContentScheme();
    static const QString &messagesBodyPath();
    static QString messageFilePath(const QString &fileName);
    static int pathIdentifier(const QString &filePath);

    static void extractMessageMetaData(const QSqlRecord& r, QMailMessageKey::Properties recordProperties, const QMailMessageKey::Properties& properties, QMailMessageMetaData* metaData);

private:
    template <typename T, typename ID> 
    class Cache
    {
    public:
        Cache(unsigned int size = 10);
        ~Cache();

        T lookup(const ID& id) const;
        void insert(const T& item);
        bool contains(const ID& id) const;
        void remove(const ID& id);
        void clear();

    private:
        QCache<quint64,T> mCache;
    };

    mutable QSqlDatabase database;
    
    mutable QMailMessageIdList lastQueryMessageResult;

    mutable Cache<QMailMessageMetaData, QMailMessageId> headerCache;
    mutable Cache<QMailFolder, QMailFolderId> folderCache;
    mutable Cache<QMailAccount, QMailAccountId> accountCache;

    mutable QList<const QMailMessageKey*> requiredTableKeys;
    mutable QList<const QMailMessageKey*> temporaryTableKeys;
    QList<const QMailMessageKey*> expiredTableKeys;

    bool inTransaction;
    mutable int lastQueryError;

    ProcessMutex *mutex;
    ProcessReadLock *readLock;

    static ProcessMutex *contentMutex;
};

template <typename ValueType>
ValueType QMailStorePrivate::extractValue(const QVariant &var, const ValueType &defaultValue)
{
    if (!qVariantCanConvert<ValueType>(var)) {
        qWarning() << "QMailStorePrivate::extractValue - Cannot convert variant to:"
#ifdef QMAILSTORE_USE_RTTI
                   << typeid(ValueType).name();
#else
                   << "requested type";
#endif
        return defaultValue;
    }

    return qVariantValue<ValueType>(var);
}


template <typename T, typename ID> 
QMailStorePrivate::Cache<T, ID>::Cache(unsigned int cacheSize)
    : mCache(cacheSize)
{
}

template <typename T, typename ID> 
QMailStorePrivate::Cache<T, ID>::~Cache()
{
}

template <typename T, typename ID> 
T QMailStorePrivate::Cache<T, ID>::lookup(const ID& id) const
{
    if (id.isValid())
        if (T* cachedItem = mCache.object(id.toULongLong()))
            return *cachedItem;

    return T();
}

template <typename T, typename ID> 
void QMailStorePrivate::Cache<T, ID>::insert(const T& item)
{
    if (item.id().isValid())
        mCache.insert(item.id().toULongLong(),new T(item));
}

template <typename T, typename ID> 
bool QMailStorePrivate::Cache<T, ID>::contains(const ID& id) const
{
    return mCache.contains(id.toULongLong());
}

template <typename T, typename ID> 
void QMailStorePrivate::Cache<T, ID>::remove(const ID& id)
{
    mCache.remove(id.toULongLong());
}

template <typename T, typename ID> 
void QMailStorePrivate::Cache<T, ID>::clear()
{
    mCache.clear();
}

#endif
