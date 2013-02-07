/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmaildisconnected.h"
#include "qmailstore.h"
#include "qmaillog.h"

/*!
    \class QMailDisconnected
    \ingroup messaginglibrary

    \preliminary
    \brief The QMailDisconnected class provides functions to work with external servers using the 
    disconnected mode of operation.
        
    Functions are provided such as moveToFolder() and flagMessage() to update the state 
    of the mail store without requiring that the update be immediately propagated to 
    any external servers affected by that update. A function rollBackUpdates() is
    provided to allow clients to roll back changes that have been applied to the mail store
    but not yet synchronized with an external server.
    
    QMailDisconnected also provides functions such as sourceKey() and 
    sourceFolderId() to query the state of a folder or message as it was at the time of 
    the last synchronization with the external server associated with the folder or message.
    
    Additionally QMailDisconnected provides functions to work with messages containing 
    disconnected state such as clearPreviousFolder(), copyPreviousFolder() and restoreMap().
    
    Finally pending disconnected operations may be propagaged to an external server, that is 
    synchronized with an external server using QMailRetrievalAction::exportUpdates().
*/


/*!
    Returns a key matching messages that are scheduled to be moved into the folder identified by \a folderId,
    when the next synchronization of the account containing that folder occurs.
    
    \sa QMailRetrievalAction::exportUpdates()
*/
QMailMessageKey QMailDisconnected::destinationKey(const QMailFolderId &folderId)
{
    QMailMessageKey result(QMailMessageKey::parentFolderId(folderId));
    result &= (~QMailMessageKey::previousParentFolderId(QMailFolderId())
              | QMailMessageKey::status(QMailMessage::LocalOnly));
    return result;
}

/*!
    Returns a key matching messages whose parent folder's identifier was equal to \a folderId,
    at the time the most recent synchronization of the message with the originating external 
    server occurred.

    \sa QMailMessageKey::parentFolderId()
*/
QMailMessageKey QMailDisconnected::sourceKey(const QMailFolderId &folderId)
{
    QMailMessageKey result(QMailMessageKey::parentFolderId(folderId));
    result &= QMailMessageKey::previousParentFolderId(QMailFolderId());
    result |= QMailMessageKey::previousParentFolderId(folderId);
    return result;
}

/*!
    Return the QMailFolderId of the folder that contained the message \a metaData at the time the most 
    recent synchronization of the message with the originating external server occurred.
    
    \sa QMailMessageMetaData::parentFolderId()
*/
QMailFolderId QMailDisconnected::sourceFolderId(const QMailMessageMetaData &metaData)
{
    QMailFolderId previousParentFolderId(metaData.previousParentFolderId());
    if (previousParentFolderId.isValid())
        return previousParentFolderId;
    
    return metaData.parentFolderId();
}


/*!
    Returns a bit mask matching all QMailMessageKey properties related to disconnected state.
*/
QMailMessageKey::Properties QMailDisconnected::parentFolderProperties()
{
    return QMailMessageKey::ParentFolderId | QMailMessageKey::PreviousParentFolderId;
}

/*!
    Clears all data related to the disconnected state of the \a message.
*/
void QMailDisconnected::clearPreviousFolder(QMailMessageMetaData *message)
{
    message->setPreviousParentFolderId(QMailFolderId());
}

/*!
    Copies all message data related to disconnected state from \a source into \a dest.
*/
void QMailDisconnected::copyPreviousFolder(const QMailMessageMetaData &source, QMailMessageMetaData *dest)
{
    dest->setPreviousParentFolderId(source.previousParentFolderId());
}

/*!
    Returns a map of originating folders for a list of messages that are in the disconnected moved state
    as identified by \a messageIds.
    
    The map contains a key for each originating folder, and for each key the value is a list of messages
    moved out of the folder.
*/
QMap<QMailFolderId, QMailMessageIdList> QMailDisconnected::restoreMap(const QMailMessageIdList &messageIds)
{
    QMap<QMailFolderId, QMailMessageIdList> result;
    QMailMessageKey key(QMailMessageKey::id(messageIds));
    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::PreviousParentFolderId);
    
    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(key, props)) {
        if (metaData.previousParentFolderId().isValid()) {
            result[metaData.previousParentFolderId()].append(metaData.id());
        }
    }
    return result;
}

static quint64 messageStatusSetMaskForFolder(QMailFolder::StandardFolder standardFolder)
{
    switch (standardFolder) {
    case QMailFolder::DraftsFolder:
        return QMailMessage::Draft;
    case QMailFolder::TrashFolder:
        return QMailMessage::Trash;
    case QMailFolder::SentFolder:
        return QMailMessage::Sent;
    case QMailFolder::JunkFolder:
        return QMailMessage::Junk;
    case QMailFolder::OutboxFolder:
        return QMailMessage::Outbox;
    case QMailFolder::InboxFolder:
         ; // Don't do anything
    }
    return 0;
}

static quint64 messageStatusUnsetMaskForFolder(QMailFolder::StandardFolder standardFolder)
{
    return messageStatusSetMaskForFolder(standardFolder)
            ^ (QMailMessage::Draft | QMailMessage::Sent | QMailMessage::Trash | QMailMessage::Junk | QMailMessage::Outbox);
}

static void syncStatusWithFolder(QMailMessageMetaData& message, QMailFolder::StandardFolder standardFolder)
{
    message.setStatus(messageStatusUnsetMaskForFolder(standardFolder), false);
    message.setStatus(messageStatusSetMaskForFolder(standardFolder), true);
}

static void syncStatusWithFolder(QMailMessageMetaData& message)
{
    Q_ASSERT(message.parentAccountId().isValid() && message.parentFolderId().isValid());

    QMailAccount messageAccount(message.parentAccountId());

    for( QMap<QMailFolder::StandardFolder, QMailFolderId>::const_iterator it(messageAccount.standardFolders().begin())
        ; it != messageAccount.standardFolders().end() ; it++)
    {
        if (message.parentFolderId() == it.value()) {
            syncStatusWithFolder(message, it.key());
        }
    }

}
/*!
    Returns true if disconnected operations have been applied to the message store since 
    the most recent synchronization with the account specified by \a mailAccountId.
    
    Otherwise returns false.
    
    \sa rollBackUpdates()
*/
bool QMailDisconnected::updatesOutstanding(const QMailAccountId &mailAccountId)
{
    QMailFolderKey accountFoldersKey(QMailFolderKey::parentAccountId(mailAccountId));
    QMailMessageKey copiedKey(QMailMessageKey::parentAccountId(mailAccountId) 
                              & QMailMessageKey::status(QMailMessage::LocalOnly));
    QMailMessageKey movedKey(QMailMessageKey::previousParentFolderId(accountFoldersKey));
    QMailMessageIdList copiedIds = QMailStore::instance()->queryMessages(copiedKey);
    QMailMessageIdList movedIds = QMailStore::instance()->queryMessages(movedKey);
    
    if (!copiedIds.isEmpty() || !movedIds.isEmpty())
        return true;
    
    QMailMessageRemovalRecordList removalRecords = QMailStore::instance()->messageRemovalRecords(mailAccountId);
    QStringList serverUidList;
    foreach (const QMailMessageRemovalRecord& r, removalRecords) {
        if (!r.serverUid().isEmpty())
            serverUidList.append(r.serverUid());
    }
    if (!serverUidList.isEmpty())
        return true;
    
    QMailMessageKey accountKey(QMailMessageKey::parentAccountId(mailAccountId));
    QMailMessageKey readStatusKey(QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Includes));
    readStatusKey &= QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Excludes);
    readStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);
    readStatusKey &= QMailMessageKey::parentFolderId(QMailFolderId(QMailFolderFwd::LocalStorageFolderId), QMailDataComparator::NotEqual);
    if (QMailStore::instance()->countMessages(accountKey & readStatusKey))
        return true;

    QMailMessageKey unreadStatusKey(QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Excludes));
    unreadStatusKey &= QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Includes);
    unreadStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);
    unreadStatusKey &= QMailMessageKey::parentFolderId(QMailFolderId(QMailFolderFwd::LocalStorageFolderId), QMailDataComparator::NotEqual);
    if (QMailStore::instance()->countMessages(accountKey & unreadStatusKey))
        return true;

    QMailMessageKey importantStatusKey(QMailMessageKey::status(QMailMessage::Important, QMailDataComparator::Includes));
    importantStatusKey &= QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Excludes);
    importantStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);
    importantStatusKey &= QMailMessageKey::parentFolderId(QMailFolderId(QMailFolderFwd::LocalStorageFolderId), QMailDataComparator::NotEqual);
    if (QMailStore::instance()->countMessages(accountKey & importantStatusKey))
        return true;
    
    QMailMessageKey unimportantStatusKey(QMailMessageKey::status(QMailMessage::Important, QMailDataComparator::Excludes));
    unimportantStatusKey &= QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Includes);
    unimportantStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);
    unimportantStatusKey &= QMailMessageKey::parentFolderId(QMailFolderId(QMailFolderFwd::LocalStorageFolderId), QMailDataComparator::NotEqual);
    if (QMailStore::instance()->countMessages(accountKey & unimportantStatusKey))
        return true;
        
    if (!QMailStore::instance()->messageRemovalRecords(mailAccountId).isEmpty())
        return true;
    
    return false;
}

/*!
    Rolls back all disconnected move and copy operations that have been applied to the 
    message store since the most recent synchronization of the message with the account 
    specified by \a mailAccountId.
    
    \sa updatesOutstanding()
*/
void QMailDisconnected::rollBackUpdates(const QMailAccountId &mailAccountId)
{
    QMailFolderKey accountFoldersKey(QMailFolderKey::parentAccountId(mailAccountId));
    QMailMessageKey copiedKey(QMailMessageKey::parentAccountId(mailAccountId) 
                              & QMailMessageKey::status(QMailMessage::LocalOnly));
    QMailMessageKey movedKey(QMailMessageKey::previousParentFolderId(accountFoldersKey));
    QMailMessageIdList copiedIds = QMailStore::instance()->queryMessages(copiedKey);
    QMailMessageIdList movedIds = QMailStore::instance()->queryMessages(movedKey);

    // remove copies
    if(!copiedIds.isEmpty()) {
        if (!QMailStore::instance()->removeMessages(QMailMessageKey::id(copiedIds))) {
            qWarning() << "Unable to rollback disconnected copies for account:" << mailAccountId;
            return;
        }
    }

    // undo moves
    foreach(const QMailMessageId& id, movedIds) {
        QMailMessageMetaData mail(id);
        mail.setParentFolderId(mail.previousParentFolderId());
        mail.setPreviousParentFolderId(QMailFolderId());
        syncStatusWithFolder(mail);           
        if (!QMailStore::instance()->updateMessage(&mail)) {
            qWarning() << "Unable to rollback disconnected moves for account:" << mailAccountId;
            return;
        }
    }

    // undo removals
   QMailMessageRemovalRecordList removalRecords = QMailStore::instance()->messageRemovalRecords(mailAccountId);
   QStringList serverUidList;
   foreach (const QMailMessageRemovalRecord& r, removalRecords) {
       if (!r.serverUid().isEmpty())
           serverUidList.append(r.serverUid());
   }
   
   if (!QMailStore::instance()->purgeMessageRemovalRecords(mailAccountId, serverUidList)) {
       qWarning() << "Unable to rollback disconnected removal records for account:" << mailAccountId;
       return;
   }
   
   // undo flag changes
   QMailMessageKey accountKey(QMailMessageKey::parentAccountId(mailAccountId));
   QMailMessageKey removedKey(accountKey & QMailMessageKey::serverUid(serverUidList));
   if (!QMailStore::instance()->updateMessagesMetaData(removedKey, QMailMessage::Removed, false)) {
       qWarning() << "Unable to rollback disconnected removed flagging for account:" << mailAccountId;
       return;
   }
   
    QMailMessageKey readStatusKey(QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Includes));
    readStatusKey &= QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Excludes);
    if (!QMailStore::instance()->updateMessagesMetaData(accountKey & readStatusKey, QMailMessage::Read, false)) {
        qWarning() << "Unable to rollback disconnected unread->read flagging for account:" << mailAccountId;
        return;
    }
    
    QMailMessageKey unreadStatusKey(QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Excludes));
    unreadStatusKey &= QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Includes);
    if (!QMailStore::instance()->updateMessagesMetaData(accountKey & unreadStatusKey, QMailMessage::Read, true)) {
        qWarning() << "Unable to rollback disconnected read->unread flagging for account:" << mailAccountId;
        return;
    }
    
    QMailMessageKey importantStatusKey(QMailMessageKey::status(QMailMessage::Important, QMailDataComparator::Includes));
    importantStatusKey &= QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Excludes);
    if (!QMailStore::instance()->updateMessagesMetaData(accountKey & importantStatusKey, QMailMessage::Important, false)) {
        qWarning() << "Unable to rollback disconnected unimportant->important flagging for account:" << mailAccountId;
        return;
    }
    
    QMailMessageKey unimportantStatusKey(QMailMessageKey::status(QMailMessage::Important, QMailDataComparator::Excludes));
    unimportantStatusKey &= QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Includes);
    if (!QMailStore::instance()->updateMessagesMetaData(accountKey & unimportantStatusKey, QMailMessage::Important, true)) {
        qWarning() << "Unable to rollback disconnected important->unimportant flagging for account:" << mailAccountId;
        return;
    }    

    QMailStore::instance()->purgeMessageRemovalRecords(mailAccountId);
}

/*!
    Disconnected moves the list of messages identified by \a ids into the standard folder \a standardFolder, setting standard 
    folder flags as appropriate.
    
    The move operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
*/
void QMailDisconnected::moveToStandardFolder(const QMailMessageIdList& ids, QMailFolder::StandardFolder standardFolder)
{
    QList<QMailMessageMetaData *> messages; // Using this for efficient update

    foreach(const QMailMessageId &id, ids) {
        QMailMessageMetaData *msg = new QMailMessageMetaData(id);
        QMailFolderId folderId(QMailAccount(msg->parentAccountId()).standardFolder(standardFolder)); // will be cached
        QMailFolder folder;
        if (folderId.isValid()) {
            folder = QMailFolder(folderId);
        }
        
        if (folderId.isValid() && folder.id().isValid()) {
            moveToFolder(msg, folderId);
            messages.append(msg);
        } else {
            delete msg;
        }
    }

    if (!messages.isEmpty()) {
        QMailStore::instance()->updateMessages(messages);
        foreach(QMailMessageMetaData *messagePointer, messages) {
            delete messagePointer;
        }
    }
}

/*!
    Disconnected moves the list of messages identified by \a ids into the folder identified by \a folderId, setting standard 
    folder flags as appropriate.

    Moving to another account is not supported.

    The move operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
*/
void QMailDisconnected::moveToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId)
{
    Q_ASSERT(folderId.isValid());
    QList<QMailMessageMetaData *> messages;  // Using this for efficient update

    foreach (const QMailMessageId &id, ids) {
        QMailMessageMetaData *msg = new QMailMessageMetaData(id);
        moveToFolder(msg, folderId);
        messages.append(msg);
    }

    if (!messages.empty()) {
        QMailStore::instance()->updateMessages(messages);
        foreach(QMailMessageMetaData *messagePointer, messages) {
            delete messagePointer;
        }
    }
}

/*!
    Disconnected updates \a message to be moved to the folder identified \a folderId. This function does NOT update the message in QMailStore,
    QMailStore::updateMessage() should be used to ensure that changes are to propagated to the server.

    Moving to another account is not supported.
*/
void QMailDisconnected::moveToFolder(QMailMessageMetaData *message, const QMailFolderId &folderId)
{
    Q_ASSERT(message);
    Q_ASSERT(folderId.isValid());
    Q_ASSERT(message->parentAccountId().isValid());
    Q_ASSERT(folderId == QMailFolder::LocalStorageFolderId ||
             message->parentAccountId() == QMailFolder(folderId).parentAccountId());

    if (message->parentFolderId() == folderId)
        return;
    if (!(message->status() & QMailMessage::LocalOnly) && !message->serverUid().isEmpty() && !message->previousParentFolderId().isValid())
        message->setPreviousParentFolderId(message->parentFolderId());

    // if the previousParentFolderId is the same as our soon-to-be-parentFolderId, we need to clear
    // it so it doesn't re-synchronise with the server
    if (folderId == message->previousParentFolderId())
        message->setPreviousParentFolderId(QMailFolderId());

    message->setRestoreFolderId(message->parentFolderId());
    message->setParentFolderId(folderId);
    syncStatusWithFolder(*message);
}

/*!
    Disconnected copies the list of messages identified by \a ids into the standard folder \a standardFolder, setting standard 
    folder flags as appropriate.
    
    The copy operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
    
    This function must only be used on messages for which the entire content of the message and all parts of the 
    message is available in the message store; otherwise the behavior is undefined.
    
    \sa QMailMessagePartContainer::contentAvailable()
*/
void QMailDisconnected::copyToStandardFolder(const QMailMessageIdList& ids, QMailFolder::StandardFolder standardFolder)
{
    QMailAccountIdList allAccounts = QMailStore::instance()->queryAccounts();

    foreach(const QMailAccountId& id, allAccounts) {
        QMailAccount account(id);
        QMailFolderId standardFolderId = account.standardFolder(standardFolder);
        if (standardFolderId.isValid())
            copyToFolder(ids,standardFolderId);
   }
}

/*!
    Disconnected copies the list of messages identified by \a ids into the folder identified by \a folderId, setting standard 
    folder flags as appropriate.

    The copy operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().

    This function must only be used on messages for which the entire content of the message and all parts of the 
    message is available in the message store; otherwise the behavior is undefined.

    \sa QMailMessagePartContainer::contentAvailable()
*/
void QMailDisconnected::copyToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId)
{
    if (!folderId.isValid())
        return;

    QMailFolder folder(folderId);

    QMailMessageKey key(QMailMessageKey::id(ids) & QMailMessageKey::parentAccountId(folder.parentAccountId()));
    QMailMessageIdList messageIds = QMailStore::instance()->queryMessages(key);
    foreach (const QMailMessageId& messageId, messageIds) {
        QMailMessage mail(messageId);
        QMailMessage copy(QMailMessage::fromRfc2822(mail.toRfc2822()));
        copy.setMessageType(QMailMessage::Email);
        copy.setPreviousParentFolderId(QMailFolderId());
        copy.setParentFolderId(folderId);
        copy.setParentAccountId(mail.parentAccountId());
        copy.setSize(mail.size());
        copy.setStatus(mail.status());
        copy.setStatus(QMailMessage::LocalOnly,true);
        copy.setStatus(QMailMessage::Removed,false);
        syncStatusWithFolder(copy);
        QMailStore::instance()->addMessage(&copy);
    }
}

/*!
    Disconnected flags the list of messages identified by \a ids, setting the flags specified by the bit mask \a setMask 
    to on and setting the flags set by the bit mask \a unsetMask to off.
    
    For example this function may be used to mark messages as important.

    The flagging operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
    
    During synchronization with the server a status message with contents \a description may be emitted.
*/
void QMailDisconnected::flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask, const QString& description)
{
    Q_UNUSED(description)
    if (setMask && !QMailStore::instance()->updateMessagesMetaData(QMailMessageKey::id(ids), setMask, true)) {
        qMailLog(Messaging) << "Unable to flag messages:" << ids;
    }

    if (unsetMask && !QMailStore::instance()->updateMessagesMetaData(QMailMessageKey::id(ids), unsetMask, false)) {
        qMailLog(Messaging) << "Unable to flag messages:" << ids;
    }
}

/*!
    Disconnected flags the list a message identified by \a id, setting the flags specified by the bit mask \a setMask 
    to on and setting the flags set by the bit mask \a unsetMask to off.
    
    For example this function may be used to mark a message as important.

    The flagging operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
    
    During synchronization with the server a status message with contents \a description may be emitted.
*/
void QMailDisconnected::flagMessage(const QMailMessageId &id, quint64 setMask, quint64 unsetMask, const QString& description)
{
    flagMessages(QMailMessageIdList() << id, setMask, unsetMask, description);
}

/*!
    Updates the QMailMessage with QMailMessageId \a id to move the message back to the
    previous folder it was contained by.

    Returns \c true if the operation completed successfully, \c false otherwise.

    \sa QMailDisconnected::moveToFolder(), QMailDisconnected::moveToStandardFolder()
*/
void QMailDisconnected::restoreToPreviousFolder(const QMailMessageId& id)
{
    return restoreToPreviousFolder(QMailMessageKey::id(id));
}

/*!
    Updates all QMailMessages identified by the key \a key to move the messages back to the
    previous folder they were contained by.
*/
void QMailDisconnected::restoreToPreviousFolder(const QMailMessageKey& key)
{
    QList<QMailMessageMetaData *> messages;  // Using this for efficient update

    QMailMessageIdList results(QMailStore::instance()->queryMessages(key));

    foreach (const QMailMessageId &id, results) {
        Q_ASSERT(id.isValid());

        QMailMessageMetaData *message = new QMailMessageMetaData(id);

        QMailFolderId restoreFolderId(message->restoreFolderId());

        // if there's no valid folder to restore to, we'll simply do nothing,
        // then leave the user to do a manual move
        if (!restoreFolderId.isValid()) {
            continue;
        }

        moveToFolder(message, restoreFolderId);

        // clear the restore folder id since moveToFolder will set it again, which we don't want
        message->setRestoreFolderId(QMailFolderId());

        messages.append(message);
    }

    if (!messages.empty()) {
        QMailStore::instance()->updateMessages(messages);
        foreach(QMailMessageMetaData *messagePointer, messages) {
            delete messagePointer;
        }
    }
}

