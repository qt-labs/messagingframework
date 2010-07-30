/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
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

static QMap<QMailFolder::StandardFolder,quint64> flagMap()
{
    static QMap<QMailFolder::StandardFolder,quint64> sFlagMap;
    if(sFlagMap.isEmpty())
    {
        sFlagMap.insert(QMailFolder::DraftsFolder,QMailMessage::Draft);
        sFlagMap.insert(QMailFolder::TrashFolder,QMailMessage::Trash);
        sFlagMap.insert(QMailFolder::SentFolder,QMailMessage::Sent);
        sFlagMap.insert(QMailFolder::JunkFolder,QMailMessage::Junk);
    }
    return sFlagMap;
}

static void syncStatusWithFolder(QMailMessageMetaData& message, QMailFolder::StandardFolder standardFolder)
{
    quint64 clearFolderStatus = message.status() &~ (QMailMessage::Draft | QMailMessage::Sent | QMailMessage::Trash | QMailMessage::Junk);
    message.setStatus(clearFolderStatus);
    message.setStatus(flagMap().value(standardFolder),true);
}

static void syncStatusWithFolder(QMailMessageMetaData& message)
{
    if (!message.parentAccountId().isValid())
        return;

    QMailAccount messageAccount(message.parentAccountId());

    foreach(QMailFolder::StandardFolder sf, flagMap().keys()) {
        if (message.parentFolderId().isValid() 
            && messageAccount.standardFolder(sf) == message.parentFolderId()) {
            syncStatusWithFolder(message, sf);
            return;
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
    if (QMailStore::instance()->countMessages(accountKey & readStatusKey))
        return true;

    QMailMessageKey unreadStatusKey(QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Excludes));
    unreadStatusKey &= QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Includes);
    unreadStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);
    if (QMailStore::instance()->countMessages(accountKey & unreadStatusKey))
        return true;

    QMailMessageKey importantStatusKey(QMailMessageKey::status(QMailMessage::Important, QMailDataComparator::Includes));
    importantStatusKey &= QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Excludes);
    importantStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);
    if (QMailStore::instance()->countMessages(accountKey & importantStatusKey))
        return true;
    
    QMailMessageKey unimportantStatusKey(QMailMessageKey::status(QMailMessage::Important, QMailDataComparator::Excludes));
    unimportantStatusKey &= QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Includes);
    unimportantStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);
    if (QMailStore::instance()->countMessages(accountKey & unimportantStatusKey))
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
}

/*!
    Disconnected moves the list of messages identified by \a ids into the standard folder \a standardFolder, setting standard 
    folder flags as appropriate.
    
    The move operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
*/
void QMailDisconnected::moveToStandardFolder(const QMailMessageIdList& ids, QMailFolder::StandardFolder standardFolder)
{
    QMailAccountIdList allAccounts = QMailStore::instance()->queryAccounts();

    foreach(const QMailAccountId& id, allAccounts) {
        QMailAccount account(id);
        QMailFolderId standardFolderId = account.standardFolder(standardFolder);
        if (standardFolderId.isValid())
            moveToFolder(ids,standardFolderId);
    }
}

/*!
    Disconnected moves the list of messages identified by \a ids into the folder identified by \a folderId, setting standard 
    folder flags as appropriate.

    The move operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
*/
void QMailDisconnected::moveToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId)
{
    if (!folderId.isValid())
        return;

    QMailFolder folder(folderId);

    if(!folder.parentAccountId().isValid())
        return;

    QMailMessageKey key(QMailMessageKey::id(ids) & QMailMessageKey::parentAccountId(folder.parentAccountId()));
    QMailMessageIdList messageIds = QMailStore::instance()->queryMessages(key);
    foreach(const QMailMessageId& messageId, messageIds) {
        QMailMessageMetaData msg(messageId);
        if (msg.parentFolderId() == folderId)
            continue;
        if (!(msg.status() & QMailMessage::LocalOnly) && !msg.serverUid().isEmpty() && !msg.previousParentFolderId().isValid())
            msg.setPreviousParentFolderId(msg.parentFolderId());
        msg.setParentFolderId(folderId);
        syncStatusWithFolder(msg);
        QMailStore::instance()->updateMessage(&msg);
    }
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
    
    For example this function may be used to mark messages as important or 'move to trash' messages.

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
    
    For example this function may be used to mark a message as important or 'move to trash' a message.

    The flagging operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
    
    During synchronization with the server a status message with contents \a description may be emitted.
*/
void QMailDisconnected::flagMessage(const QMailMessageId &id, quint64 setMask, quint64 unsetMask, const QString& description)
{
    flagMessages(QMailMessageIdList() << id, setMask, unsetMask, description);
}

