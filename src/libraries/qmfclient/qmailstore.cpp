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

#include "qmailstore.h"
#include "qmailstore_p.h"
#include <QThreadStorage>

/*!
    \class QMailStore

    \preliminary
    \brief The QMailStore class represents the main interface for storage and retrieval
    of messages and folders on the message store.

    \ingroup messaginglibrary

    The QMailStore class is accessed through a singleton interface and provides functions
    for adding, updating and deleting of QMailAccounts, QMailFolders, QMailThreads and
    QMailMessages on the message store.

    QMailStore also provides functions for querying and counting of QMailFolders, QMailAccounts,
    QMailThreads and QMailMessages when used in conjunction with QMailFolderKey and
    QMailAccountKey, QMailThreadKey and QMailMessageKey classes.

    If a QMailStore operation fails, the lastError() function will return an error code
    value indicating the failure mode encountered.  A successful operation will set the
    lastError() result to QMailStore::NoError.

    Messaging accounts are represented by QMailAccountId objects.  The data associated with
    accounts is separated into two components: QMailAccount objects hold account properties
    exported to mail store client applications, and QMailAccountConfiguration objects hold
    data used only by the messageserver and the protocol plugins it loads.

    Account objects are accessed via the account(), accountConfiguration(), countAccounts()
    and queryAccounts() functions.  Accounts in the mail store can be manipulated via the
    addAccount(), updateAccount() and removeAccount() functions.  Mail store manipulations
    affecting accounts are reported via the accountsAdded(), accountsUpdated(),
    accountContentsModified() and accountsRemoved() signals.

    Thread (a.k.a. conversation) objects are accessed via the thread(), countThreads()
    and queryThreads() functions.  Accounts in the mail store can be manipulated via the
    addThread(), updateThread() and removeThread() functions.  Mail store manipulations
    affecting accounts are reported via the threadsAdded(), threadsUpdated(),
    threadsContentsModified() and threadsRemoved() signals.



    Fixed logical groupings of message are modelled as folders, represented by QMailFolderId objects.
    The data associated with folders is held by instances of the QMailFolder class.

    Folder objects are accessed via the folder(), countFolders() and queryFolders() functions.
    Folders in the mail store can be manipulated via the addFolder(), updateFolder() and
    removeFolder() functions.  Mail store manipulations affecting folders are reported via
    the foldersAdded(), foldersUpdated(), folderContentsModified() and foldersRemoved() signals.

    Messages in the mail store are represented by QMailMessageId objects.  The data associated
    with a message can be retrieved in two forms: QMailMessageMetaData objects contain only
    the meta data fields associated with a message, and QMailMessage objects contain both
    the meta data fields and the message content proper.

    Message objects are accessed via the message(), messageMetaData(), countMessages()
    and queryMessages() functions.  Additionally, the messagesMetaData() function can be
    used to retrieve subsets of meta data pertaining to a set of messages.  Messages in
    the mail store can be manipulated via the addMessage(), updateMessage() and removeMessage()
    functions.  Multiple messages can have their meta data fields updated together via
    the updateMessagesMetaData() function.  Mail store manipulations affecting messages are
    reported via the messagesAdded(), messagesUpdated(), messageContentsModified() and
    messagesRemoved() signals.

    Messages that have been removed can be represented by removal records, which persist
    only to assist in keeping mail store content synchronized with the content of
    an external message source.  QMailMessageRemovalRecord objects can be accessed
    via the messageRemovalRecords() function.

    \sa QMailAccount, QMailFolder, QMailMessage
*/

/*!
    \enum QMailStore::InitializationState
    This enum defines the initialization states that the mail store can assume.

    \value Uninitialized        The mail store has not yet been instantiated and initialized.
    \value InitializationFailed The mail store has been instantiated and initization was unsuccessful.
    \value Initialized          The mail store has been instantiated and successfully initialized.
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
    \value ContentNotRemoved    The operation failed only because content could not be removed from storage.
    \value FrameworkFault       The operation failed because the mail store encountered an error in performing the operation.
    \value StorageInaccessible  The operation failed because the mail storage mechanism cannot be accessed by the mail store.
*/

/*!
    Constructs a new QMailStore object and opens the message store database.
*/
QMailStore::QMailStore()
    : d(new QMailStorePrivate(this, QMailAccountManager::newManager(this)))
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
    Returns the initialization state of the QMailStore.
*/
QMailStore::InitializationState QMailStore::initializationState()
{
    return QMailStorePrivate::initializationState();
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
    Adds a new QMailThread object \a t into the message store, performing
    respective integrity checks. Returns \c true if the operation
    completed successfully, \c false otherwise.
*/
bool QMailStore::addThread(QMailThread *t)
{
    QMailThreadIdList addedThreadIds;

    d->setLastError(NoError);

    if (!d->addThread(t, &addedThreadIds))
        return false;

    emitThreadNotification(Added, addedThreadIds);

    return true;
}

/*!
    Adds a new QMailMessage object \a msg into the message store, performing
    respective integrity checks. Returns \c true if the operation
    completed successfully, \c false otherwise.
*/
bool QMailStore::addMessage(QMailMessage* msg)
{
    return addMessages(QList<QMailMessage*>() << msg);
}

/*!
    Adds a new QMailMessageMetaData object \a metaData into the message store, performing
    respective integrity checks. Returns \c true if the operation completed
    successfully, \c false otherwise.
*/
bool QMailStore::addMessage(QMailMessageMetaData* metaData)
{
    return addMessages(QList<QMailMessageMetaData*>() << metaData);
}

/*!
    Adds a new QMailMessage object into the message store for each entry in
    the list \a messages, performing all respective integrity checks.
    Returns \c true if the operation completed successfully, \c false otherwise.
*/
bool QMailStore::addMessages(const QList<QMailMessage*>& messages)
{
    QMailMessageIdList addedMessageIds;
    QMailThreadIdList addedThreadIds;
    QMailMessageIdList updatedMessageIds;
    QMailThreadIdList updatedThreadIds;
    QMailFolderIdList modifiedFolderIds;
    QMailThreadIdList modifiedThreadIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->addMessages(messages, &addedMessageIds, &addedThreadIds, &updatedMessageIds, &updatedThreadIds, &modifiedFolderIds, &modifiedThreadIds, &modifiedAccountIds))
        return false;

    emitMessageNotification(Added, addedMessageIds);
    emitThreadNotification(Added, addedThreadIds);
    emitMessageDataNotification(Added, dataList(messages, addedMessageIds));
    emitMessageDataNotification(Updated, dataList(messages, updatedMessageIds));
    emitMessageNotification(Updated, updatedMessageIds);
    emitFolderNotification(ContentsModified, modifiedFolderIds);
    emitThreadNotification(ContentsModified, modifiedThreadIds);
    emitThreadNotification(Updated, updatedThreadIds);
    emitAccountNotification(ContentsModified, modifiedAccountIds);

    return true;
}

/*!
    Adds a new QMailMessageMetData object into the message store for each entry in
    the list \a messages, performing all respective integrity checks.
    Returns \c true if the operation completed successfully, \c false otherwise.
*/
bool QMailStore::addMessages(const QList<QMailMessageMetaData*>& messages)
{
    QMailMessageIdList addedMessageIds;
    QMailThreadIdList addedThreadIds;
    QMailMessageIdList updatedMessageIds;
    QMailThreadIdList updatedThreadIds;
    QMailFolderIdList modifiedFolderIds;
    QMailThreadIdList modifiedThreadIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->addMessages(messages, &addedMessageIds, &addedThreadIds, &updatedMessageIds, &updatedThreadIds, &modifiedFolderIds, &modifiedThreadIds, &modifiedAccountIds))
        return false;

    emitMessageNotification(Added, addedMessageIds);
    emitMessageNotification(Updated, updatedMessageIds);
    emitMessageDataNotification(Added, dataList(messages, addedMessageIds));
    emitThreadNotification(Added, addedThreadIds);
    emitMessageDataNotification(Updated, dataList(messages, updatedMessageIds));
    emitThreadNotification(Updated, updatedThreadIds);
    emitFolderNotification(ContentsModified, modifiedFolderIds);
    emitThreadNotification(ContentsModified, modifiedThreadIds);
    emitAccountNotification(ContentsModified, modifiedAccountIds);
    return true;
}

/*!
    Removes a QMailAccount with QMailAccountId \a id from the store.  Also removes any
    folder, message and message removal records associated with the removed account.
    Returns \c true if the operation completed successfully, \c false otherwise.

    Note: Using a QMailAccount instance after it has been removed from the store will
    result in undefined behavior.
*/
bool QMailStore::removeAccount(const QMailAccountId& id)
{
    return removeAccounts(QMailAccountKey::id(id));
}

/*!
    Removes all QMailAccounts identified by the key \a key from the store. Also removes
    any folder, message and message removal records associated with the removed account.
    Returns \c true if the operation completed successfully, \c false otherwise.

    Note: Using a QMailAccount instance after it has been removed from the store will
    result in undefined behavior.
*/
bool QMailStore::removeAccounts(const QMailAccountKey& key)
{
    QMailAccountIdList deletedAccountIds;
    QMailFolderIdList deletedFolderIds;
    QMailThreadIdList deletedThreadIds;
    QMailMessageIdList deletedMessageIds;
    QMailMessageIdList updatedMessageIds;
    QMailFolderIdList modifiedFolderIds;
    QMailThreadIdList modifiedThreadIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->removeAccounts(key, &deletedAccountIds, &deletedFolderIds, &deletedThreadIds, &deletedMessageIds,
                           &updatedMessageIds, &modifiedFolderIds, &modifiedThreadIds, &modifiedAccountIds))
        return false;

    emitRemovalRecordNotification(Removed, deletedAccountIds);
    emitMessageNotification(Removed, deletedMessageIds);
    emitThreadNotification(Removed, deletedThreadIds);
    emitFolderNotification(Removed, deletedFolderIds);
    emitAccountNotification(Removed, deletedAccountIds);
    emitMessageNotification(Updated, updatedMessageIds);
    emitFolderNotification(ContentsModified, modifiedFolderIds);
    emitThreadNotification(ContentsModified, modifiedThreadIds);
    emitAccountNotification(ContentsModified, modifiedAccountIds);
    return true;
}

/*!
    Removes a QMailFolder with QMailFolderId \a id from the message store. Also removes any
    sub-folders of the identified folder, and any messages contained in any of the
    folders removed.  If \a option is QMailStore::CreateRemovalRecord then removal
    records will be created for each removed message.
    Returns \c true if the operation completed successfully, \c false otherwise.

    Note: Using a QMailFolder instance after it has been removed from the store will
    result in undefined behavior.
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

    Note: Using a QMailFolder instance after it has been removed from the store will
    result in undefined behavior.
*/
bool QMailStore::removeFolders(const QMailFolderKey& key, QMailStore::MessageRemovalOption option)
{
    QMailFolderIdList deletedFolderIds;
    QMailMessageIdList deletedMessageIds;
    QMailThreadIdList deletedThreadIds;
    QMailMessageIdList updatedMessageIds;
    QMailFolderIdList modifiedFolderIds;
    QMailThreadIdList modifiedThreadIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->removeFolders(key, option, &deletedFolderIds, &deletedMessageIds, &deletedThreadIds, &updatedMessageIds, &modifiedFolderIds, &modifiedThreadIds, &modifiedAccountIds))
        return false;

    emitRemovalRecordNotification(Added, modifiedAccountIds);
    emitMessageNotification(Removed, deletedMessageIds);
    emitThreadNotification(Removed, deletedThreadIds);
    emitFolderNotification(Removed, deletedFolderIds);
    emitMessageNotification(Updated, updatedMessageIds);
    emitFolderNotification(ContentsModified, modifiedFolderIds);
    emitThreadNotification(ContentsModified, modifiedThreadIds);
    emitAccountNotification(ContentsModified, modifiedAccountIds);
    return true;
}

/*!
    Removes all QMailThreads identified by the id \a id from the message store. If \a option is
    QMailStore::CreateRemovalRecord then removal records will be created for each removed thread.
    Returns \c true if the operation completed successfully, \c false otherwise.

    Note: Using a QMailThreads instance after it has been removed from the store will
    result in undefined behavior.
*/
bool QMailStore::removeThread(const QMailThreadId &id, QMailStore::MessageRemovalOption option)
{
    return removeThreads(QMailThreadKey::id(id), option);
}


/*!
    Removes all QMailThreads identified by the key \a key from the message store. If \a option is
    QMailStore::CreateRemovalRecord then removal records will be created for each removed thread.
    Returns \c true if the operation completed successfully, \c false otherwise.

    Note: Using a QMailThreads instance after it has been removed from the store will
    result in undefined behavior.
*/
bool QMailStore::removeThreads(const QMailThreadKey& key, QMailStore::MessageRemovalOption option)
{
    QMailThreadIdList deletedThreadIds;
    QMailMessageIdList deletedMessageIds;
    QMailMessageIdList updatedMessageIds;
    QMailFolderIdList modifiedFolderIds;
    QMailThreadIdList modifiedThreadIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->removeThreads(key, option, &deletedThreadIds, &deletedMessageIds, &updatedMessageIds, &modifiedFolderIds, &modifiedThreadIds, &modifiedAccountIds))
        return false;

    emitRemovalRecordNotification(Added, modifiedAccountIds);
    emitMessageNotification(Removed, deletedMessageIds);
    emitThreadNotification(Removed, deletedThreadIds);
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

    Note: Using a QMailMessage instance after it has been removed from the store will
    result in undefined behavior.
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

    Note: Using a QMailMessage instance after it has been removed from the store will
    result in undefined behavior.
*/
bool QMailStore::removeMessages(const QMailMessageKey& key, QMailStore::MessageRemovalOption option)
{
    QMailMessageIdList deletedMessageIds;
    QMailThreadIdList deletedThreadIds;
    QMailMessageIdList updatedMessageIds;
    QMailFolderIdList modifiedFolderIds;
    QMailThreadIdList modifiedThreadIds;
    QMailAccountIdList modifiedAccountIds;

    d->setLastError(NoError);
    if (!d->removeMessages(key, option, &deletedMessageIds, &deletedThreadIds, &updatedMessageIds, &modifiedFolderIds, &modifiedThreadIds, &modifiedAccountIds))
        return false;

    emitRemovalRecordNotification(Added, modifiedAccountIds);
    emitMessageNotification(Removed, deletedMessageIds);
    emitMessageNotification(Updated, updatedMessageIds);
    emitFolderNotification(ContentsModified, modifiedFolderIds);
    // FIXME: use updatedThreadIds instead of modifiedThreadIds
    // to emit signal about updated threads. However, do so we should write
    // one more bind impl, otherwise we've got too much args for bind in QMailStorePrivate.
    emitThreadNotification(Updated, modifiedThreadIds);
    emitThreadNotification(ContentsModified, modifiedThreadIds);
    emitThreadNotification(Removed, deletedThreadIds);
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
    Updates existing QMailThread \a t in the message store.
    Returns \c true if the operation completed successfully, \c false otherwise.
*/
bool QMailStore::updateThread(QMailThread* t)
{
    QMailThreadIdList updatedThreads;
    d->setLastError(NoError);
    if (!d->updateThread(t, &updatedThreads))
        return false;

    emitThreadNotification(Updated, updatedThreads);
    return true;
}

/*!
    Ensure mail store is durably written to the file system.

    Returns \c true if the operation completed successfully, \c false otherwise.
*/
bool QMailStore::ensureDurability()
{
    d->setLastError(NoError);
    if (!d->ensureDurability())
        return false;

    return true;
}

/*!
    Updates the existing QMailMessage \a msg on the message store.
    Returns \c true if the operation completed successfully, or \c false otherwise.
*/
bool QMailStore::updateMessage(QMailMessage* msg)
{
    return updateMessages(QList<QMailMessage*>() << msg);
}

/*!
    Updates the meta data of the existing message on the message store, to match \a metaData.
    Returns \c true if the operation completed successfully, or \c false otherwise.
*/
bool QMailStore::updateMessage(QMailMessageMetaData* metaData)
{
    return updateMessages(QList<QMailMessageMetaData*>() << metaData);
}

/*!
    Updates the existing QMailMessage in the message store, for each message listed in \a messages.
    Returns \c true if the operation completed successfully, or \c false otherwise.
*/
bool QMailStore::updateMessages(const QList<QMailMessage*>& messages)
{
    QList<QPair<QMailMessageMetaData*, QMailMessage*> > msgs;
    foreach (QMailMessage* message, messages) {
        msgs.append(qMakePair(static_cast<QMailMessageMetaData*>(message), message));
    }

    return updateMessages(msgs);
}

/*!
    Updates the meta data of the existing message in the message store, to match each
    of the messages listed in \a messages.
    Returns \c true if the operation completed successfully, or \c false otherwise.
*/
bool QMailStore::updateMessages(const QList<QMailMessageMetaData*>& messages)
{
    QList<QPair<QMailMessageMetaData*, QMailMessage*> > msgs;
    foreach (QMailMessageMetaData* metaData, messages) {
        msgs.append(qMakePair(metaData, reinterpret_cast<QMailMessage*>(0)));
    }

    return updateMessages(msgs);
}

/*! \internal */
bool QMailStore::updateMessages(const QList<QPair<QMailMessageMetaData*, QMailMessage*> >& messages)
{
    QMailMessageIdList updatedMessages;
    QMailThreadIdList modifiedThreads;
    QMailMessageIdList modifiedMessages;
    QMailFolderIdList modifiedFolders;
    QMailAccountIdList modifiedAccounts;

    d->setLastError(NoError);
    if (!d->updateMessages(messages, &updatedMessages, &modifiedThreads, &modifiedMessages, &modifiedFolders, &modifiedAccounts))
        return false;

    QList<QMailMessageMetaData*> data;
    typedef QPair<QMailMessageMetaData*, QMailMessage*> Pair;
    for (const Pair& pair : messages) {
        Q_ASSERT (pair.first);
        data.append(pair.first);
    }

    emitMessageNotification(Updated, updatedMessages);
    // FIXME: use updatedThreadIds instead of modifiedThreadIds
    // to emit signal about updated threads. However, do so we should write
    // one more bind impl, otherwise we've got too much args for bind in QMailStorePrivate.
    emitThreadNotification(Updated, modifiedThreads);
    emitThreadNotification(ContentsModified, modifiedThreads);
    emitMessageNotification(ContentsModified, modifiedMessages);
    emitMessageDataNotification(Updated, dataList(data, updatedMessages));
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
    QMailThreadIdList deletedThreads;
    QMailThreadIdList modifiedThreads;
    QMailFolderIdList modifiedFolders;
    QMailAccountIdList modifiedAccounts;

    d->setLastError(NoError);
    if (!d->updateMessagesMetaData(key, properties, data, &updatedMessages, &deletedThreads, &modifiedThreads, &modifiedFolders, &modifiedAccounts))
        return false;

    emitMessageNotification(Updated, updatedMessages);
    emitMessageDataNotification(updatedMessages, properties, data);
    emitThreadNotification(Removed, deletedThreads);
    // FIXME: use updatedThreadIds instead of modifiedThreadIds
    // to emit signal about updated threads. However, do so we should write
    // one more bind impl, otherwise we've got too much args for bind in QMailStorePrivate.
    emitThreadNotification(Updated, modifiedThreads);
    emitThreadNotification(ContentsModified, modifiedThreads);
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
    QMailThreadIdList modifiedThreads;

    d->setLastError(NoError);
    if (!d->updateMessagesMetaData(key, status, set, &updatedMessages, &modifiedThreads, &modifiedFolders, &modifiedAccounts))
        return false;

    emitMessageNotification(Updated, updatedMessages);
    emitMessageDataNotification(updatedMessages, status, set);
    emitThreadNotification(Updated, modifiedThreads);
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
    Returns the count of the number of threads which pass the
    filtering criteria defined in QMailThreadKey \a key. If
    key is empty a count of all folders is returned.
*/
int QMailStore::countThreads(const QMailThreadKey& key) const
{
    d->setLastError(NoError);
    return d->countThreads(key);
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
    If \a limit is non-zero, then no more than \a limit matching account IDs should be
    returned.
    If \a offset is non-zero, then the first \a offset matching IDs should be omitted
    from the returned list.

    Note: if the implementation cannot support the \a limit and \a offset parameters,
    it should not attempt to perform a query where either of these values is non-zero;
    instead, it should return an empty list and set lastError() to QMailStore::NotYetImplemented.
*/
const QMailAccountIdList QMailStore::queryAccounts(const QMailAccountKey& key,
                                                   const QMailAccountSortKey& sortKey,
                                                   uint limit,
                                                   uint offset) const
{
    d->setLastError(NoError);
    return d->queryAccounts(key, sortKey, limit, offset);
}

/*!
    Returns the \l{QMailFolderId}s of folders in the message store. If \a key is not empty
    only folders matching the parameters set by \a key will be returned, otherwise
    all folder identifiers will be returned.
    If \a sortKey is not empty, the identifiers will be sorted by the parameters set
    by \a sortKey.
    If \a limit is non-zero, then no more than \a limit matching folder IDs should be
    returned.
    If \a offset is non-zero, then the first \a offset matching IDs should be omitted
    from the returned list.

    Note: if the implementation cannot support the \a limit and \a offset parameters,
    it should not attempt to perform a query where either of these values is non-zero;
    instead, it should return an empty list and set lastError() to QMailStore::NotYetImplemented.
*/
const QMailFolderIdList QMailStore::queryFolders(const QMailFolderKey& key,
                                                 const QMailFolderSortKey& sortKey,
                                                 uint limit,
                                                 uint offset) const
{
    d->setLastError(NoError);
    return d->queryFolders(key, sortKey, limit, offset);
}

/*!
    Returns the \l{QMailMessageId}s of messages in the message store. If \a key is not empty
    only messages matching the parameters set by \a key will be returned, otherwise
    all message identifiers will be returned.
    If \a sortKey is not empty, the identifiers will be sorted by the parameters set
    by \a sortKey.
    If \a limit is non-zero, then no more than \a limit matching message IDs should be
    returned.
    If \a offset is non-zero, then the first \a offset matching IDs should be omitted
    from the returned list.

    Note: if the implementation cannot support the \a limit and \a offset parameters,
    it should not attempt to perform a query where either of these values is non-zero;
    instead, it should return an empty list and set lastError() to QMailStore::NotYetImplemented.
*/
const QMailMessageIdList QMailStore::queryMessages(const QMailMessageKey& key,
                                                   const QMailMessageSortKey& sortKey,
                                                   uint limit,
                                                   uint offset) const
{
    d->setLastError(NoError);
    return d->queryMessages(key, sortKey, limit, offset);
}


/*!
    Returns the \l{QMailThreadId}s of threads in the message store. If \a key is not empty
    only threads matching the parameters set by \a key will be returned, otherwise
    all thread identifiers will be returned.
    If \a sortKey is not empty, the identifiers will be sorted by the parameters set
    by \a sortKey.
    If \a limit is non-zero, then no more than \a limit matching thread IDs should be
    returned.
    If \a offset is non-zero, then the first \a offset matching IDs should be omitted
    from the returned list.

    Note: if the implementation cannot support the \a limit and \a offset parameters,
    it should not attempt to perform a query where either of these values is non-zero;
    instead, it should return an empty list and set lastError() to QMailStore::NotYetImplemented.
*/
const QMailThreadIdList QMailStore::queryThreads(const QMailThreadKey &key, const QMailThreadSortKey &sortKey, uint limit, uint offset) const
{
    d->setLastError(NoError);
    return d->queryThreads(key, sortKey, limit, offset);
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
   Returns the QMailThread defined by QMailThreadId \a id from the store.
*/
QMailThread QMailStore::thread(const QMailThreadId &id) const
{
    d->setLastError(NoError);
    return d->thread(id);
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
    that have been removed from local storage. Records are retrieved for messages whose account IDs
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
    Locks QMailStore, preventing all write operations from taking place. Will block until all write operations have
    completed and it can get a lock. Read-only operations will be permitted as normal.To resume normal operation unlock()
    must be called.

    Note: This method only needs to be used in exceptional circumstances, such as when directly accessing
        the content of QMailStore files e.g. for backing up data.

    \sa unlock()
*/

void QMailStore::lock()
{
    d->setLastError(NoError);
    return d->lock();
}

/*!
    Unlocks QMailStore, allowing write operations to resume. This must only be used after a lock() call
    and from the same process in which lock() was called.

    \sa lock()
*/

void QMailStore::unlock()
{
    d->setLastError(NoError);
    d->unlock();
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
    Sets the list of accounts currently retrieving from external sources to be \a ids.

    \sa retrievalInProgress()
*/
void QMailStore::setRetrievalInProgress(const QMailAccountIdList &ids)
{
    if (d->setRetrievalInProgress(ids)) {
        emitRetrievalInProgress(ids);
    }
}

/*!
    Sets the list of accounts currently transmitting to external sources to be \a ids.

    \sa transmissionInProgress()
*/
void QMailStore::setTransmissionInProgress(const QMailAccountIdList &ids)
{
    if (d->setTransmissionInProgress(ids)) {
        emitTransmissionInProgress(ids);
    }
}

/*!
    Forces any queued event notifications to immediately be synchronously emitted.

    Any events occurring before flushIpcNotifications() is invoked will be processed by
    recipient processes before any IPC packets generated after the invocation.

    \sa isIpcConnectionEstablished(), disconnectIpc(), reconnectIpc()
*/
void QMailStore::flushIpcNotifications()
{
    d->flushIpcNotifications();
}

/*!
    Returns true if a connection to the messageserver is established.

    The messageserver is used to notify a QMF client in one process of changes
    that have been made by QMF clients in other processes. For example
    QMailStore::messagesAdded() signal, will inform a client in one process
    when a client in another process, such as the messageserver, has added
    messages to the message store.

    \sa flushIpcNotifications(), disconnectIpc(), reconnectIpc(), QMailServiceAction
*/
bool QMailStore::isIpcConnectionEstablished() const
{
    return d->isIpcConnectionEstablished();
}

/*!
    Disconnect from messageserver

    Useful for reducing battery consumption when the client application is not
    visible to the end user. Calling any getter functions like messages() for
    instance may result in incorrect content after a call to disconnect() since
    this store client will not be notified of any database change made by other
    clients. Respectively, calling any setter functions like updateMessages()
    will not notify any other clients. The host code is responsible to call
    reconnectIpc() before using any getter or setter functions.

    \sa flushIpcNotifications(), isIpcConnectionEstablished(), reconnectIpc()
*/
void QMailStore::disconnectIpc()
{
    d->disconnectIpc();
}

/*!
    Reconnect to messageserver

    \sa flushIpcNotifications(), isIpcConnectionEstablished(), disconnectIpc()
*/
void QMailStore::reconnectIpc()
{
    d->reconnectIpc();
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
QMap<QString, QString> QMailStore::messageCustomFields(const QMailMessageId &id)
{
    return d->messageCustomFields(id);
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
    Q_ASSERT(!ids.contains(QMailAccountId()));
    if (!ids.isEmpty()) {
        // Ensure there are no duplicates in the list
        const QSet<QMailAccountId> uids(ids.constBegin(), ids.constEnd());
        QMailAccountIdList idList(uids.constBegin(), uids.constEnd());

        d->notifyAccountsChange(type, idList);

        switch (type) {
        case Added:
            emit accountsAdded(idList);
            break;

        case Removed:
            emit accountsRemoved(idList);
            break;

        case Updated:
            emit accountsUpdated(idList);
            break;

        case ContentsModified:
            emit accountContentsModified(idList);
            break;
        }
    }
}

/*! \internal */
void QMailStore::emitFolderNotification(ChangeType type, const QMailFolderIdList &ids)
{
    Q_ASSERT(!ids.contains(QMailFolderId()));
    if (!ids.isEmpty()) {
        // Ensure there are no duplicates in the list
        const QSet<QMailFolderId> uids(ids.constBegin(), ids.constEnd());
        QMailFolderIdList idList(uids.constBegin(), uids.constEnd());

        d->notifyFoldersChange(type, idList);

        switch (type) {
        case Added:
            emit foldersAdded(idList);
            break;

        case Removed:
            emit foldersRemoved(idList);
            break;

        case Updated:
            emit foldersUpdated(idList);
            break;

        case ContentsModified:
            emit folderContentsModified(idList);
            break;
        }
    }
}

/*! \internal */
void QMailStore::emitThreadNotification(ChangeType type, const QMailThreadIdList &ids)
{
    Q_ASSERT(!ids.contains(QMailThreadId()));
    if (!ids.isEmpty()) {
        // Ensure there are no duplicates in the list
        const QSet<QMailThreadId> uids(ids.constBegin(), ids.constEnd());
        QMailThreadIdList idList(uids.constBegin(), uids.constEnd());

        d->notifyThreadsChange(type, idList);

        switch (type) {
        case Added:
            emit threadsAdded(idList);
            break;

        case Removed:
            emit threadsRemoved(idList);
            break;

        case Updated:
            emit threadsUpdated(idList);
            break;

        case ContentsModified:
            emit threadContentsModified(idList);
            break;
        }
    }
}

/*! \internal */
void QMailStore::emitMessageNotification(ChangeType type, const QMailMessageIdList &ids)
{
    Q_ASSERT(!ids.contains(QMailMessageId()));
    if (!ids.isEmpty()) {
        // Ensure there are no duplicates in the list
        const QSet<QMailMessageId> uids(ids.constBegin(), ids.constEnd());
        QMailMessageIdList idList(uids.constBegin(), uids.constEnd());

        d->notifyMessagesChange(type, idList);

        switch (type) {
        case Added:
            emit messagesAdded(idList);
            break;

        case Removed:
            emit messagesRemoved(idList);
            break;

        case Updated:
            emit messagesUpdated(idList);
            break;

        case ContentsModified:
            emit messageContentsModified(idList);
            break;
        }
    }
}

/*! \internal */
void QMailStore::emitMessageDataNotification(ChangeType type, const QMailMessageMetaDataList &data)
{
    if (!data.isEmpty()) {
        // Ensure there are no duplicates in the list
        d->notifyMessagesDataChange(type, data);

        switch (type) {
        case Added:
            emit messageDataAdded(data);
            break;

        case Updated:
            emit messageDataUpdated(data);
            break;

        default:
            Q_ASSERT (false);
        }

    }
}

/*! \internal */
void QMailStore::emitMessageDataNotification(const QMailMessageIdList &ids, const QMailMessageKey::Properties &properties,
                                             const QMailMessageMetaData &data)
{
    if (!ids.isEmpty()) {
        d->notifyMessagesDataChange(ids, properties, data);
        emit messagePropertyUpdated(ids, properties, data);
    }
}

/*! \internal */
void QMailStore::emitMessageDataNotification(const QMailMessageIdList& ids, quint64 status, bool set)
{
    if (!ids.isEmpty()) {
        d->notifyMessagesDataChange(ids, status, set);
        emit messageStatusUpdated(ids, status, set);
    }
}

/*! \internal */
void QMailStore::emitRemovalRecordNotification(ChangeType type, const QMailAccountIdList &ids)
{
    if (!ids.isEmpty()) {
        // Ensure there are no duplicates in the list
        const QSet<QMailAccountId> uids(ids.constBegin(), ids.constEnd());
        QMailAccountIdList idList(uids.constBegin(), uids.constEnd());

        d->notifyMessageRemovalRecordsChange(type, idList);

        switch (type) {
        case Added:
            emit messageRemovalRecordsAdded(idList);
            break;

        case Removed:
            emit messageRemovalRecordsRemoved(idList);
            break;

        case Updated:
        case ContentsModified:
            break;
        }
    }
}

/*! \internal */
void QMailStore::emitRetrievalInProgress(const QMailAccountIdList &ids)
{
    d->notifyRetrievalInProgress(ids);

    emit retrievalInProgress(ids);
}

/*! \internal */
void QMailStore::emitTransmissionInProgress(const QMailAccountIdList &ids)
{
    d->notifyTransmissionInProgress(ids);

    emit transmissionInProgress(ids);
}

/*! \internal */
QMailMessageMetaData QMailStore::dataToTransfer(const QMailMessageMetaData* message)
{
    Q_ASSERT (message);
    QMailMessageMetaData metaData;
    // init all the fields except custom fields
    metaData.setId(message->id());
    metaData.setParentFolderId(message->parentFolderId());
    metaData.setMessageType(message->messageType());
    metaData.setFrom(message->from());
    metaData.setSubject(message->subject());
    metaData.setDate(message->date());
    metaData.setReceivedDate(message->receivedDate());
    metaData.setRecipients(message->recipients());
    metaData.setStatus(message->status());
    metaData.setParentAccountId(message->parentAccountId());
    metaData.setServerUid(message->serverUid());
    metaData.setSize(message->size());
    metaData.setContent(message->content());
    metaData.setPreviousParentFolderId(message->previousParentFolderId());
    metaData.setContentScheme(message->contentScheme());
    metaData.setContentIdentifier(message->contentIdentifier());
    metaData.setInResponseTo(message->inResponseTo());
    metaData.setResponseType(message->responseType());
    metaData.setPreview(message->preview());
    metaData.setCopyServerUid(message->copyServerUid());
    metaData.setRestoreFolderId(message->restoreFolderId());
    metaData.setListId(message->listId());
    metaData.setRfcId(message->rfcId());
    metaData.setParentThreadId(message->parentThreadId());
    metaData.setUnmodified();

    return metaData;
}

/*! \internal */
QMailMessageMetaDataList QMailStore::dataList(const QList<QMailMessage*>& messages, const QMailMessageIdList& ids)
{
    QMailMessageMetaDataList data;

    foreach (QMailMessage* message, messages) {
        Q_ASSERT (message);
        if (ids.contains(message->id())) {
            data.append(dataToTransfer(message));
        }
    }

    return data;
}

/*! \internal */
QMailMessageMetaDataList QMailStore::dataList(const QList<QMailMessageMetaData*>& messages, const QMailMessageIdList& ids)
{
    QMailMessageMetaDataList data;

    foreach (QMailMessageMetaData* message, messages) {
        Q_ASSERT (message);
        if (ids.contains(message->id())) {
            data.append(dataToTransfer(message));
        }
    }

    return data;
}

class QMailStoreInstanceData
{
public:
    QMailStoreInstanceData()
    {
        mailStore = nullptr;
        init = false;
    }

    ~QMailStoreInstanceData()
    {
        delete mailStore;
    }

    QMailStore *mailStore;
    bool init;
};

Q_GLOBAL_STATIC(QThreadStorage<QMailStoreInstanceData *>, mailStoreDataInstance)

/*!
    Returns the single instance of the QMailStore class.

    If necessary, the store will be instantiated and initialized.

    \sa initializationState()
*/
QMailStore* QMailStore::instance()
{
    if (!mailStoreDataInstance()->hasLocalData()) {
        mailStoreDataInstance()->setLocalData(new QMailStoreInstanceData);
    }
    QMailStoreInstanceData* instance = mailStoreDataInstance()->localData();

    if (!instance->init) {
        instance->init = true;

        instance->mailStore = new QMailStore;
        QMailStore *store = instance->mailStore;
        store->d->initialize();
        if (initializationState() == QMailStore::InitializationFailed) {
            delete store->d;
            store->d = new QMailStoreNullImplementation(store);
        }
    }
    return instance->mailStore;
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
    \fn void QMailStore::folderContentsModified(const QMailFolderIdList& ids)

    Signal that is emitted when changes to messages in the mail store
    affect the content of the folders in the list \a ids.

    \sa messagesAdded(), messagesUpdated(), messagesRemoved()
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
    \fn void QMailStore::retrievalInProgress(const QMailAccountIdList& ids)

    Signal that is emitted when the set of accounts currently retrieving from
    external sources is modified to \a ids.  Accounts listed in \a ids are likely
    to be the source of numerous mail store signals; some clients may wish to
    ignore updates associated with these accounts whilst they are engaged in retrieving.

    \sa transmissionInProgress()
*/

/*!
    \fn void QMailStore::transmissionInProgress(const QMailAccountIdList& ids)

    Signal that is emitted when the set of accounts currently transmitting to
    external sources is modified to \a ids.  Accounts listed in \a ids are likely
    to be the source of numerous mail store signals; some clients may wish to
    ignore updates associated with these accounts whilst they are engaged in transmitting.

    \sa retrievalInProgress()
*/

/*!
    \fn void QMailStore::messageDataAdded(const QMailMessageMetaDataList &data)

    Signal that is emitted when messages are added to the mail store
    using addMessage(), addMessages() or an overload of one of these functions.
    \a data is a list of the metadata of the added messages.

    \sa addMessages()
*/

/*!
    \fn void QMailStore::messageDataUpdated(const QMailMessageMetaDataList &data)

    Signal that is emitted when messages within the mail store are updated using
    using updateMessages(), updateMessages(), addMessage(), addMessages() or an overload of one of these functions.
    \a data is a list of the metadata of the updated messages.
*/

/*!
    \fn void QMailStore::messagePropertyUpdated(const QMailMessageIdList& ids,
                                    const QMailMessageKey::Properties& properties,
                                    const QMailMessageMetaData& data);

    Signal that is emitted when messages within the mail store are updated using
    \l {updateMessagesMetaData()}{updateMessagesMetaData(const QMailMessageKey&, const QMailMessageKey::Properties& properties, const QMailMessageMetaData& data)}.
    \a ids is a list of ids of messages that have been updated, message properties defined in \a properties have been updated using
    the respective element contained in the \a data.
*/

/*!
    \fn void QMailStore::messageStatusUpdated(const QMailMessageIdList& ids, quint64 status, bool set);

    Signal that is emitted when messages within the mail store are updated using
    \l {updateMessagesMetaData()}{updateMessagesMetaData(const QMailMessageKey&, quint64, bool)}.
    \a ids is a list of ids of messages that have been updated, \a status is the status flags set according to \a set.
*/

/*!
    \fn void QMailStore::threadsAdded(const QMailThreadIdList& ids)

    Signal that is emitted when the threads in the list \a ids are
    added to the store.

    \sa threadsRemoved(), threadsUpdated()
*/

/*!
    \fn void QMailStore::threadsRemoved(const QMailThreadIdList& ids)

    Signal that is emitted when the threads in the list \a ids are
    removed from the store.

    \sa threadsAdded(), threadsUpdated()
*/

/*!
    \fn void QMailStore::threadsUpdated(const QMailThreadIdList& ids)

    Signal that is emitted when the threads in the list \a ids are
    updated within the store.

    \sa threadsAdded(), threadsRemoved()
*/

/*!
    \fn void QMailStore::threadContentsModified(const QMailThreadIdList& ids)

    Signal that is emitted when changes to messages in the mail store
    affect the content of the threads in the list \a ids.

    \sa messagesAdded(), messagesUpdated(), messagesRemoved()
*/

/*!
    \fn bool QMailStore::ipcConnectionEstablished()

    Signal that is emitted when a connection to the messageserver is established.

    \sa isIpcConnectionEstablished()
*/

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailStore::MessageRemovalOption)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailStore::ChangeType)
