/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

// Needed to give friend access to the function defined by Q_GLOBAL_STATIC
#define QMAILSTOREINSTANCE_DEFINED_HERE
#include "qmailstore.h"
#include "qmailstore_p.h"

/*!
    \class QMailStore
    \inpublicgroup QtMessagingModule

    \preliminary
    \brief The QMailStore class represents the main interface for storage and retrieval
    of messages and folders on the message store.

    \ingroup messaginglibrary

    The QMailStore class is accessed through a singleton interface and provides functions 
    for adding, updating and deleting of QMailAccounts, QMailFolders and QMailMessages on the message store.

    QMailStore also provides functions for querying and counting of QMailFolders, QMailAccounts and QMailMessages
    when used in conjunction with QMailMessageKey, QMailFolderKey and QMailAccountKey classes.

    \sa QMailMessage, QMailFolder, QMailMessageKey, QMailFolderKey, QMailAccountKey
*/

/*!
    \enum QMailStore::ReturnOption
    This enum defines the meta data list return option for QMailStore::messagesMetaData()

    \value ReturnAll        Return all meta data objects that match the selection criteria, including duplicates.
    \value ReturnDistinct   Return distinct meta data objects that match the selection criteria, excluding duplicates.
*/

/*!
    \enum QMailStore::MessageRemovalOption

    Defines whether or not a QMailMessageRemovalRecord is created upon message removal. 

    \value NoRemovalRecord Do not create a QMailMessageRemovalRecord upon message removal. 
    \value CreateRemovalRecord Create a QMailMessageRemovalRecord upon message removal. 
*/

/*!
    \enum QMailStore::ChangeType

    Defines the type of change made to an object contained by the mail store.

    \value Added    A new entity was added to the store.
    \value Removed  An existing entity was removed from the store.
    \value Updated  An existing entity was modified.
    \value ContentsModified An existing entity was affected by a change to a different entity.
*/

/*!
    \enum QMailStore::ErrorCode

    Defines the result of attempting to perform a mail store operation.

    \value NoError              The operation was successfully performed.
    \value InvalidId            The operation failed due to the specification of an invalid identifier.
    \value ConstraintFailure    The operation failed due to the violation of a database constraint.
    \value ContentInaccessible  The operation failed because the content data cannot be accessed by the mail store.
    \value NotYetImplemented    The operation failed because the mail store does not yet implement the operation.
    \value FrameworkFault       The operation failed because the mail store encountered an error in performing the operation.
*/

/*!
    Constructs a new QMailStore object and opens the message store database.
*/
QMailStore::QMailStore()
    : d(new QMailStorePrivate(this))
{
}

/*!
    Destroys this QMailStore object.
*/
QMailStore::~QMailStore()
{
    delete d; d = 0;
}

/*!
    Returns true if the QMailStore object was correctly initialized.
*/
bool QMailStore::initialized() const
{
    return QMailStore::storeInitialized();
}

/*!
    Returns true if the QMailStore was correctly initialized.
*/
bool QMailStore::storeInitialized()
{
    return QMailStorePrivate::initialized();
}

/*!
    Returns the code of the last error condition reported by the QMailStore.
*/
QMailStore::ErrorCode QMailStore::lastError() const
{
    return d->lastError();
}

/*!
    Adds a new QMailAccount object \a account into the messsage store, with the
    configuration details optionally specified by \a config.
    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::addAccount(QMailAccount* account, QMailAccountConfiguration* config)
{
    QMailAccountIdList addedAccountIds;

    d->setLastError(NoError);
    if (!d->addAccount(account, config, &addedAccountIds))
        return false;

    emitAccountNotification(Added, addedAccountIds);
    return true;
}

/*!
    Adds a new QMailFolder object \a folder into the message store, performing
    respective integrity checks. Returns \c true if the operation
    completed successfully, \c false otherwise. 
*/
bool QMailStore::addFolder(QMailFolder* folder)
{
    QMailFolderIdList addedFolderIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->addFolder(folder, &addedFolderIds, &modifiedAccountIds))
        return false;

    emitFolderNotification(Added, addedFolderIds);
    emitAccountNotification(ContentsModified, modifiedAccountIds);
    return true;
}

/*!
    Adds a new QMailMessage object \a msg into the message store, performing
    respective integrity checks. Returns \c true if the operation
    completed successfully, \c false otherwise. 
*/
bool QMailStore::addMessage(QMailMessage* msg)
{
    QMailMessageIdList addedMessageIds;
    QMailMessageIdList updatedMessageIds;
    QMailFolderIdList modifiedFolderIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->addMessage(msg, &addedMessageIds, &updatedMessageIds, &modifiedFolderIds, &modifiedAccountIds))
        return false;

    emitMessageNotification(Added, addedMessageIds);
    emitMessageNotification(Updated, updatedMessageIds);
    emitFolderNotification(ContentsModified, modifiedFolderIds);
    emitAccountNotification(ContentsModified, modifiedAccountIds);
    return true;
}

/*!
    Adds a new QMailMessageMetaData object \a metaData into the message store, performing
    respective integrity checks. Returns \c true if the operation completed 
    successfully, \c false otherwise. 
*/
bool QMailStore::addMessage(QMailMessageMetaData* metaData)
{
    QMailMessageIdList addedMessageIds;
    QMailMessageIdList updatedMessageIds;
    QMailFolderIdList modifiedFolderIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->addMessage(metaData, &addedMessageIds, &updatedMessageIds, &modifiedFolderIds, &modifiedAccountIds))
        return false;

    emitMessageNotification(Added, addedMessageIds);
    emitMessageNotification(Updated, updatedMessageIds);
    emitFolderNotification(ContentsModified, modifiedFolderIds);
    emitAccountNotification(ContentsModified, modifiedAccountIds);
    return true;
}

/*!
    Removes a QMailAccount with QMailAccountId \a id from the store.  Also removes any 
    folder, message and message removal records associated with the removed account.
    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::removeAccount(const QMailAccountId& id) 
{
    return removeAccounts(QMailAccountKey::id(id));
}

/*!
    Removes all QMailAccounts identified by the key \a key from the store. Also removes 
    any folder, message and message removal records associated with the removed account.
    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::removeAccounts(const QMailAccountKey& key)
{
    QMailAccountIdList deletedAccountIds;
    QMailFolderIdList deletedFolderIds;
    QMailMessageIdList deletedMessageIds;
    QMailMessageIdList updatedMessageIds;
    QMailFolderIdList modifiedFolderIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->removeAccounts(key, &deletedAccountIds, &deletedFolderIds, &deletedMessageIds, &updatedMessageIds, &modifiedFolderIds, &modifiedAccountIds))
        return false;

    emitRemovalRecordNotification(Removed, deletedAccountIds);
    emitMessageNotification(Removed, deletedMessageIds);
    emitFolderNotification(Removed, deletedFolderIds);
    emitAccountNotification(Removed, deletedAccountIds);
    emitMessageNotification(Updated, updatedMessageIds);
    emitFolderNotification(ContentsModified, modifiedFolderIds);
    emitAccountNotification(ContentsModified, modifiedAccountIds);
    return true;
}

/*!
    Removes a QMailFolder with QMailFolderId \a id from the message store. Also removes any 
    sub-folders of the identified folder, and any messages contained in any of the 
    folders removed.  If \a option is QMailStore::CreateRemovalRecord then removal 
    records will be created for each removed message.
    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::removeFolder(const QMailFolderId& id, QMailStore::MessageRemovalOption option)
{
    // Remove the identified folder and any sub-folders
    QMailFolderKey idKey(QMailFolderKey::id(id));
    QMailFolderKey subKey(QMailFolderKey::ancestorFolderIds(id, QMailDataComparator::Includes));

    return removeFolders(idKey | subKey, option);
}

/*!
    Removes all QMailFolders identified by the key \a key from the message store. Also
    removes any sub-folders of the removed folders, and any messages contained in any of
    the folders removed.  If \a option is QMailStore::CreateRemovalRecord then removal 
    records will be created for each removed message.
    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::removeFolders(const QMailFolderKey& key, QMailStore::MessageRemovalOption option)
{
    QMailFolderIdList deletedFolderIds;
    QMailMessageIdList deletedMessageIds;
    QMailMessageIdList updatedMessageIds;
    QMailFolderIdList modifiedFolderIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->removeFolders(key, option, &deletedFolderIds, &deletedMessageIds, &updatedMessageIds, &modifiedFolderIds, &modifiedAccountIds))
        return false;

    emitRemovalRecordNotification(Added, modifiedAccountIds);
    emitMessageNotification(Removed, deletedMessageIds);
    emitFolderNotification(Removed, deletedFolderIds);
    emitMessageNotification(Updated, updatedMessageIds);
    emitFolderNotification(ContentsModified, modifiedFolderIds);
    emitAccountNotification(ContentsModified, modifiedAccountIds);
    return true;
}

/*!
    Removes a QMailMessage with QMailMessageId \a id from the message store. If \a option is 
    QMailStore::CreateRemovalRecord then a removal record will be created for the
    removed message.
    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::removeMessage(const QMailMessageId& id, QMailStore::MessageRemovalOption option)
{
    return removeMessages(QMailMessageKey::id(id), option);
}

/*!
    Removes all QMailMessages identified by the key \a key from the message store.
    If \a option is QMailStore::CreateRemovalRecord then removal records will be 
    created for each removed message.
    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::removeMessages(const QMailMessageKey& key, QMailStore::MessageRemovalOption option)
{
    QMailMessageIdList deletedMessageIds;
    QMailMessageIdList updatedMessageIds;
    QMailAccountIdList modifiedAccountIds;
    QMailFolderIdList modifiedFolderIds;

    d->setLastError(NoError);
    if (!d->removeMessages(key, option, &deletedMessageIds, &updatedMessageIds, &modifiedFolderIds, &modifiedAccountIds))
        return false;

    emitRemovalRecordNotification(Added, modifiedAccountIds);
    emitMessageNotification(Removed, deletedMessageIds);
    emitMessageNotification(Updated, updatedMessageIds);
    emitFolderNotification(ContentsModified, modifiedFolderIds);
    emitAccountNotification(ContentsModified, modifiedAccountIds);
    return true;
}

/*!
    Updates the existing QMailAccount \a account on the store, with the 
    configuration details optionally specified by \a config.
    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::updateAccount(QMailAccount* account, QMailAccountConfiguration *config)
{
    QMailAccountIdList updatedAccounts;

    d->setLastError(NoError);
    if (!d->updateAccount(account, config, &updatedAccounts))
        return false;

    emitAccountNotification(Updated, updatedAccounts);
    return true;
}

/*!
    Updates the configuration details of the referenced account to contain \a config.
    Returns \c true if the operation completed successfully, \c false otherwise. 

    \sa QMailAccountConfiguration::id()
*/
bool QMailStore::updateAccountConfiguration(QMailAccountConfiguration *config)
{
    QMailAccountIdList updatedAccounts;

    d->setLastError(NoError);
    if (!d->updateAccountConfiguration(config, &updatedAccounts))
        return false;

    emitAccountNotification(Updated, updatedAccounts);
    return true;
}

/*!
    Updates the existing QMailFolder \a folder on the message store.
    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::updateFolder(QMailFolder* folder)
{
    QMailFolderIdList updatedFolders;
    QMailAccountIdList modifiedAccounts;

    d->setLastError(NoError);
    if (!d->updateFolder(folder, &updatedFolders, &modifiedAccounts))
        return false;

    emitFolderNotification(Updated, updatedFolders);
    emitAccountNotification(ContentsModified, modifiedAccounts);
    return true;
}

/*!
    Updates the existing QMailMessage \a msg on the message store.
    Returns \c true if the operation completed successfully, or \c false otherwise. 
*/
bool QMailStore::updateMessage(QMailMessage* msg)
{
    return updateMessage(msg, msg);
}

/*!
    Updates the meta data of the existing message on the message store, to match \a metaData.
    Returns \c true if the operation completed successfully, or \c false otherwise. 
*/
bool QMailStore::updateMessage(QMailMessageMetaData* metaData)
{
    return updateMessage(metaData, NULL);
}

/*! \internal */
bool QMailStore::updateMessage(QMailMessageMetaData* metaData, QMailMessage* mail)
{
    QMailMessageIdList updatedMessages;
    QMailFolderIdList modifiedFolders;
    QMailAccountIdList modifiedAccounts;
    bool modifiedContent;

    d->setLastError(NoError);
    if (!d->updateMessage(metaData, mail, &updatedMessages, &modifiedFolders, &modifiedAccounts, &modifiedContent))
        return false;

    if (modifiedContent)
        emitMessageNotification(ContentsModified, updatedMessages);
    emitMessageNotification(Updated, updatedMessages);
    emitFolderNotification(ContentsModified, modifiedFolders);
    emitAccountNotification(ContentsModified, modifiedAccounts);
    return true;
}

/*!
    Updates the message properties defined in \a properties to match the respective element
    contained in the \a data, for all messages which match the criteria defined by \a key.

    Returns \c true if the operation completed successfully, or \c false otherwise. 
*/
bool QMailStore::updateMessagesMetaData(const QMailMessageKey& key,
                                        const QMailMessageKey::Properties& properties,
                                        const QMailMessageMetaData& data) 
{
    QMailMessageIdList updatedMessages;
    QMailFolderIdList modifiedFolders;
    QMailAccountIdList modifiedAccounts;

    d->setLastError(NoError);
    if (!d->updateMessagesMetaData(key, properties, data, &updatedMessages, &modifiedFolders, &modifiedAccounts))
        return false;

    emitMessageNotification(Updated, updatedMessages);
    emitFolderNotification(ContentsModified, modifiedFolders);
    emitAccountNotification(ContentsModified, modifiedAccounts);
    return true;
}

/*!
    Updates message status flags set in \a status according to \a set,
    for messages which match the criteria defined by \a key.

    Returns \c true if the operation completed successfully, or \c false otherwise. 
*/
bool QMailStore::updateMessagesMetaData(const QMailMessageKey& key, quint64 status, bool set)
{
    QMailMessageIdList updatedMessages;
    QMailFolderIdList modifiedFolders;
    QMailAccountIdList modifiedAccounts;

    d->setLastError(NoError);
    if (!d->updateMessagesMetaData(key, status, set, &updatedMessages, &modifiedFolders, &modifiedAccounts))
        return false;

    emitMessageNotification(Updated, updatedMessages);
    emitFolderNotification(ContentsModified, modifiedFolders);
    emitAccountNotification(ContentsModified, modifiedAccounts);
    return true;
}

/*!
    Returns the count of the number of accounts which pass the 
    filtering criteria defined in QMailAccountKey \a key. If 
    key is empty a count of all accounts is returned.
*/
int QMailStore::countAccounts(const QMailAccountKey& key) const
{
    d->setLastError(NoError);
    return d->countAccounts(key);
}

/*!
    Returns the count of the number of folders which pass the 
    filtering criteria defined in QMailFolderKey \a key. If 
    key is empty a count of all folders is returned.
*/
int QMailStore::countFolders(const QMailFolderKey& key) const
{
    d->setLastError(NoError);
    return d->countFolders(key);
}

/*!
    Returns the count of the number of messages which pass the 
    filtering criteria defined in QMailMessageKey \a key. If 
    key is empty a count of all messages is returned.
*/
int QMailStore::countMessages(const QMailMessageKey& key) const
{
    d->setLastError(NoError);
    return d->countMessages(key);
}

/*!
    Returns the total size of the messages which pass the 
    filtering criteria defined in QMailMessageKey \a key. If 
    key is empty the total size of all messages is returned.
*/
int QMailStore::sizeOfMessages(const QMailMessageKey& key) const
{
    d->setLastError(NoError);
    return d->sizeOfMessages(key);
}

/*!
    Returns the \l{QMailAccountId}s of accounts in the store. If \a key is not empty 
    only accounts matching the parameters set by \a key will be returned, otherwise 
    all accounts identifiers will be returned.
    If \a sortKey is not empty, the identifiers will be sorted by the parameters set 
    by \a sortKey.
*/
const QMailAccountIdList QMailStore::queryAccounts(const QMailAccountKey& key,
                                                   const QMailAccountSortKey& sortKey) const
{
    d->setLastError(NoError);
    return d->queryAccounts(key, sortKey);
}

/*!
    Returns the \l{QMailFolderId}s of folders in the message store. If \a key is not empty 
    only folders matching the parameters set by \a key will be returned, otherwise 
    all folder identifiers will be returned.
    If \a sortKey is not empty, the identifiers will be sorted by the parameters set 
    by \a sortKey.
*/
const QMailFolderIdList QMailStore::queryFolders(const QMailFolderKey& key,
                                                 const QMailFolderSortKey& sortKey) const
{
    d->setLastError(NoError);
    return d->queryFolders(key, sortKey);
}

/*!
    Returns the \l{QMailMessageId}s of messages in the message store. If \a key is not empty 
    only messages matching the parameters set by \a key will be returned, otherwise 
    all message identifiers will be returned.
    If \a sortKey is not empty, the identifiers will be sorted by the parameters set 
    by \a sortKey.
*/
const QMailMessageIdList QMailStore::queryMessages(const QMailMessageKey& key, 
                                                   const QMailMessageSortKey& sortKey) const
{
    d->setLastError(NoError);
    return d->queryMessages(key, sortKey);
}

/*!
   Returns the QMailAccount defined by the QMailAccountId \a id from the store.
 */
QMailAccount QMailStore::account(const QMailAccountId& id) const
{
    d->setLastError(NoError);
    return d->account(id);
}

/*!
   Returns the configuration details of the QMailAccount defined by the QMailAccountId \a id from the store.
 */
QMailAccountConfiguration QMailStore::accountConfiguration(const QMailAccountId& id) const
{
    d->setLastError(NoError);
    return d->accountConfiguration(id);
}

/*!
   Returns the QMailFolder defined by the QMailFolderId \a id from the store.
*/
QMailFolder QMailStore::folder(const QMailFolderId& id) const
{
    d->setLastError(NoError);
    return d->folder(id);
}

/*!
   Returns the QMailMessage defined by the QMailMessageId \a id from the store.
*/
QMailMessage QMailStore::message(const QMailMessageId& id) const
{
    d->setLastError(NoError);
    return d->message(id);
}

/*!
   Returns the QMailMessage defined by the unique identifier \a uid from the account with id \a accountId.
*/
QMailMessage QMailStore::message(const QString& uid, const QMailAccountId& accountId) const
{
    d->setLastError(NoError);
    return d->message(uid, accountId);
}

/*!
   Returns the meta data for the message identified by the QMailMessageId \a id from the store.
*/
QMailMessageMetaData QMailStore::messageMetaData(const QMailMessageId& id) const
{
    d->setLastError(NoError);
    return d->messageMetaData(id);
}

/*!
   Returns the meta data for the message identified by the unique identifier \a uid from the account with id \a accountId.
*/
QMailMessageMetaData QMailStore::messageMetaData(const QString& uid, const QMailAccountId& accountId) const
{
    d->setLastError(NoError);
    return d->messageMetaData(uid, accountId);
}

/*!
    Retrieves a list of QMailMessageMetaData objects containing meta data elements specified by 
    \a properties, for messages which match the criteria defined by \a key. If \a option is 
    \c ReturnAll then duplicate objects are included in the list; otherwise
    duplicate objects are excluded from the returned list.

    Returns a list of QMailMessageMetaData objects if successfully completed, or an empty list for 
    an error or no data.

    Note: Custom fields cannot be queried by this function.
*/
const QMailMessageMetaDataList QMailStore::messagesMetaData(const QMailMessageKey& key,
                                                            const QMailMessageKey::Properties& properties,
                                                            ReturnOption option) const
{
    d->setLastError(NoError);
    return d->messagesMetaData(key, properties, option);
}

/*!
    Retrieves a list of QMailMessageRemovalRecord objects containing information about messages
    that have been removed from local storage. Records are retrived for messages whose account IDs 
    match \a accountId and optionally, whose folder IDs match \a folderId.
    This information is primarily for synchronization of local changes to remote message storage 
    services such as IMAP servers.

    Returns a list of QMailMessageRemovalRecord objects if successfully completed, or an empty list for 
    an error or no data.
*/
const QMailMessageRemovalRecordList QMailStore::messageRemovalRecords(const QMailAccountId& accountId,
                                                                      const QMailFolderId& folderId) const
{
    d->setLastError(NoError);
    return d->messageRemovalRecords(accountId, folderId);
}

/*!
    Erases message deletion records from the account with id \a accountId and 
    server uid listed in \a serverUids.  If serverUids is empty, all message deletion
    records for the specified account are deleted.

    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::purgeMessageRemovalRecords(const QMailAccountId& accountId, const QStringList& serverUids)
{
    d->setLastError(NoError);
    if (!d->purgeMessageRemovalRecords(accountId, serverUids))
        return false;

    emitRemovalRecordNotification(Removed, QMailAccountIdList() << accountId);
    return true;
}

/*!
    Updates the QMailMessage with QMailMessageId \a id to move the message back to the
    previous folder it was contained by.

    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::restoreToPreviousFolder(const QMailMessageId& id)
{
    return restoreToPreviousFolder(QMailMessageKey::id(id));
}

/*!
    Updates all QMailMessages identified by the key \a key to move the messages back to the
    previous folder they were contained by.

    Returns \c true if the operation completed successfully, \c false otherwise. 
*/
bool QMailStore::restoreToPreviousFolder(const QMailMessageKey& key)
{
    QMailMessageIdList updatedMessages;
    QMailFolderIdList modifiedFolders;
    QMailAccountIdList modifiedAccounts;

    d->setLastError(NoError);
    if (!d->restoreToPreviousFolder(key, &updatedMessages, &modifiedFolders, &modifiedAccounts))
        return false;

    emitMessageNotification(Updated, updatedMessages);
    emitFolderNotification(ContentsModified, modifiedFolders);
    emitAccountNotification(ContentsModified, modifiedAccounts);
    return true;
}

/*!
    Registers a status flag for QMailAccount objects, with the identifier \a name.
    Returns true if the flag is already registered, or if it is successfully registered; otherwise returns false.

    \sa accountStatusMask()
*/
bool QMailStore::registerAccountStatusFlag(const QString& name)
{
    d->setLastError(NoError);
    return d->registerAccountStatusFlag(name);
}

/*!
    Returns the status bitmask needed to test the result of QMailAccount::status() 
    against the QMailAccount status flag registered with the identifier \a name.

    \sa registerAccountStatusFlag(), QMailAccount::statusMask()
*/
quint64 QMailStore::accountStatusMask(const QString& name) const
{
    d->setLastError(NoError);
    return d->accountStatusMask(name);
}

/*!
    Registers a status flag for QMailFolder objects, with the identifier \a name.
    Returns true if the flag is already registered, or if it is successfully registered; otherwise returns false.

    \sa folderStatusMask()
*/
bool QMailStore::registerFolderStatusFlag(const QString& name)
{
    d->setLastError(NoError);
    return d->registerFolderStatusFlag(name);
}

/*!
    Returns the status bitmask needed to test the result of QMailFolder::status() 
    against the QMailFolder status flag registered with the identifier \a name.

    \sa registerFolderStatusFlag(), QMailFolder::statusMask()
*/
quint64 QMailStore::folderStatusMask(const QString& name) const
{
    d->setLastError(NoError);
    return d->folderStatusMask(name);
}

/*!
    Registers a status flag for QMailMessage objects, with the identifier \a name.
    Returns true if the flag is already registered, or if it is successfully registered; otherwise returns false.

    \sa messageStatusMask()
*/
bool QMailStore::registerMessageStatusFlag(const QString& name)
{
    d->setLastError(NoError);
    return d->registerMessageStatusFlag(name);
}

/*!
    Returns the status bitmask needed to test the result of QMailMessage::status() 
    against the QMailMessage status flag registered with the identifier \a name.

    \sa registerMessageStatusFlag(), QMailMessage::statusMask()
*/
quint64 QMailStore::messageStatusMask(const QString& name) const
{
    d->setLastError(NoError);
    return d->messageStatusMask(name);
}

/*!
    Forces any queued event notifications to immediately be synchronously emitted, and processed
    synchronously by recipient processes.

    Any events occurring before flushIpcNotifications() is invoked will be processed by 
    recipient processes before any IPC packets generated after the invocation.
*/
void QMailStore::flushIpcNotifications()
{
    d->flushIpcNotifications();
}

/*!
    Returns true if the running process is in the act of emitting an asynchronous QMailStore 
    signal caused by another process.  This can only be true when called from a slot
    invoked by a QMailStore signal.
*/
bool QMailStore::asynchronousEmission() const
{
    return d->asynchronousEmission();
}

/*! \internal */
void QMailStore::clearContent()
{
    d->clearContent();
}

/*! \internal */
void QMailStore::emitErrorNotification(QMailStore::ErrorCode code)
{
    emit errorOccurred(code);
}

/*! \internal */
void QMailStore::emitAccountNotification(ChangeType type, const QMailAccountIdList &ids)
{
    if (!ids.isEmpty()) {
        d->notifyAccountsChange(type, ids);

        switch (type) {
        case Added:
            emit accountsAdded(ids);
            break;

        case Removed:
            emit accountsRemoved(ids);
            break;

        case Updated:
            emit accountsUpdated(ids);
            break;

        case ContentsModified:
            emit accountContentsModified(ids);
            break;
        }
    }
}

/*! \internal */
void QMailStore::emitFolderNotification(ChangeType type, const QMailFolderIdList &ids)
{
    if (!ids.isEmpty()) {
        d->notifyFoldersChange(type, ids);

        switch (type) {
        case Added:
            emit foldersAdded(ids);
            break;

        case Removed:
            emit foldersRemoved(ids);
            break;

        case Updated:
            emit foldersUpdated(ids);
            break;

        case ContentsModified:
            emit folderContentsModified(ids);
            break;
        }
    }
}

/*! \internal */
void QMailStore::emitMessageNotification(ChangeType type, const QMailMessageIdList &ids)
{
    if (!ids.isEmpty()) {
        d->notifyMessagesChange(type, ids);

        switch (type) {
        case Added:
            emit messagesAdded(ids);
            break;

        case Removed:
            emit messagesRemoved(ids);
            break;

        case Updated:
            emit messagesUpdated(ids);
            break;

        case ContentsModified:
            emit messageContentsModified(ids);
            break;
        }
    }
}

/*! \internal */
void QMailStore::emitRemovalRecordNotification(ChangeType type, const QMailAccountIdList &ids)
{
    if (!ids.isEmpty()) {
        d->notifyMessageRemovalRecordsChange(type, ids);

        switch (type) {
        case Added:
            emit messageRemovalRecordsAdded(ids);
            break;

        case Removed:
            emit messageRemovalRecordsRemoved(ids);
            break;

        case Updated:
        case ContentsModified:
            break;
        }
    }
}

Q_GLOBAL_STATIC(QMailStore,QMailStoreInstance);

/*!
    Returns an instance of the QMailStore object.
*/

QMailStore* QMailStore::instance()
{
    static bool init = false;
    if (!init) {
        init = true;
        QMailStoreInstance()->d->initStore();
    }
    return QMailStoreInstance();
}

/*!
    \fn void QMailStore::accountsAdded(const QMailAccountIdList& ids)

    Signal that is emitted when the accounts in the list \a ids are
    added to the store.

    \sa accountsRemoved(), accountsUpdated()
*/

/*!
    \fn void QMailStore::accountsRemoved(const QMailAccountIdList& ids)

    Signal that is emitted when the accounts in the list \a ids are
    removed from the store.

    \sa accountsAdded(), accountsUpdated()
*/

/*!
    \fn void QMailStore::accountsUpdated(const QMailAccountIdList& ids)

    Signal that is emitted when the accounts in the list \a ids are
    updated within the store.

    \sa accountsAdded(), accountsRemoved()
*/

/*!
    \fn void QMailStore::accountContentsModified(const QMailAccountIdList& ids)

    Signal that is emitted when changes to messages and folders in the mail store
    affect the content of the accounts in the list \a ids.

    \sa messagesAdded(), messagesUpdated(), messagesRemoved(), foldersAdded(), foldersUpdated(), foldersRemoved()
*/

/*!
    \fn void QMailStore::messagesAdded(const QMailMessageIdList& ids)

    Signal that is emitted when the messages in the list \a ids are
    added to the mail store.

    \sa messagesRemoved(), messagesUpdated()
*/

/*!
    \fn void QMailStore::messagesRemoved(const QMailMessageIdList& ids)

    Signal that is emitted when the messages in the list \a ids are
    removed from the mail store.

    \sa messagesAdded(), messagesUpdated()
*/

/*!
    \fn void QMailStore::messagesUpdated(const QMailMessageIdList& ids)

    Signal that is emitted when the messages in the list \a ids are
    updated within the mail store.

    \sa messagesAdded(), messagesRemoved()
*/

/*!
    \fn void QMailStore::messageContentsModified(const QMailMessageIdList& ids)

    Signal that is emitted when the content of the messages in list \a ids is updated.

    \sa messagesUpdated()
*/

/*!
    \fn void QMailStore::errorOccurred(QMailStore::ErrorCode code)

    Signal that is emitted when an error is encountered in processing a QMailStore operation.
    The error condition is indeicated by \a code.

    \sa lastError()
*/

/*!
    \fn void QMailStore::foldersAdded(const QMailFolderIdList& ids)

    Signal that is emitted when the folders in the list \a ids are
    added to the mail store.

    \sa foldersRemoved(), foldersUpdated()
*/

/*!
    \fn void QMailStore::foldersRemoved(const QMailFolderIdList& ids)

    Signal that is emitted when the folders in the list \a ids are
    removed from the mail store.

    \sa foldersAdded(), foldersUpdated()
*/

/*!
    \fn void QMailStore::foldersUpdated(const QMailFolderIdList& ids)

    Signal that is emitted when the folders in the list \a ids are
    updated within the mail store.

    \sa foldersAdded(), foldersRemoved()
*/

/*!
    \fn void QMailStore::messageRemovalRecordsAdded(const QMailAccountIdList& ids)

    Signal that is emitted when QMailMessageRemovalRecords are added to the store, 
    affecting the accounts listed in \a ids.

    \sa messageRemovalRecordsRemoved()
*/

/*!
    \fn void QMailStore::messageRemovalRecordsRemoved(const QMailAccountIdList& ids)

    Signal that is emitted when QMailMessageRemovalRecords are removed from the store, 
    affecting the accounts listed in \a ids.

    \sa messageRemovalRecordsAdded()
*/

/*!
    \fn void QMailStore::folderContentsModified(const QMailFolderIdList& ids)

    Signal that is emitted when changes to messages in the mail store
    affect the content of the folders in the list \a ids.

    \sa messagesAdded(), messagesUpdated(), messagesRemoved()
*/

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailStore::MessageRemovalOption)

