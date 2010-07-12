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

#include "imapstrategy.h"
#include "imapclient.h"
#include "imapconfiguration.h"
#include <longstream_p.h>
#include <qobject.h>
#include <qmaillog.h>
#include <qmailaccount.h>
#include <qmailstore.h>
#include <qmailaccountconfiguration.h>
#include <qmailmessage.h>
#include <qmailnamespace.h>
#include <limits.h>
#include <QDir>


static const int MetaDataFetchFlags = F_Uid | F_Date | F_Rfc822_Size | F_Rfc822_Header | F_BodyStructure;
static const int ContentFetchFlags = F_Uid | F_Rfc822_Size | F_Rfc822;

static QString stripFolderPrefix(const QString &str)
{
    QString result;
    int index;
    if ((index = str.lastIndexOf(UID_SEPARATOR)) != -1)
        return str.mid(index + 1);
    return str;
}

static QStringList stripFolderPrefix(const QStringList &list)
{
    QStringList result;
    foreach(const QString &uid, list) {
        result.append(stripFolderPrefix(uid));
    }
    return result;
}

static QMailFolderId serverFolderId(const QMailMessageMetaData &metaData)
{
    QMailFolderId previousParentFolderId = metaData.previousParentFolderId();
    if (previousParentFolderId.isValid())
        return previousParentFolderId;
    
    return metaData.parentFolderId();
}

static QStringList inFirstAndSecond(const QStringList &first, const QStringList &second)
{
    // TODO: this may be more efficient if we convert both inputs to sets and perform set operations
    QStringList result;

    foreach (const QString &value, first)
        if (second.contains(value))
            result.append(value);

    return result;
}

static QStringList inFirstButNotSecond(const QStringList &first, const QStringList &second)
{
    QStringList result;

    foreach (const QString &value, first)
        if (!second.contains(value))
            result.append(value);

    return result;
}

static bool messageSelectorLessThan(const MessageSelector &lhs, const MessageSelector &rhs)
{
    // Any full message sorts before any message element
    if (lhs._properties.isEmpty() && !rhs._properties.isEmpty()) {
        return true;
    } else if (rhs._properties.isEmpty() && !lhs._properties.isEmpty()) {
        return false;
    }

    // Any UID value sorts before a non-UID value
    if ((lhs._uid != 0) && (rhs._uid == 0)) {
        return true;
    } else if ((rhs._uid != 0) && (lhs._uid == 0)) {
        return false;
    } 

    if (lhs._uid != 0) {
        if (lhs._uid != rhs._uid) {
            return (lhs._uid < rhs._uid);
        }
    } else {
        if (lhs._messageId.toULongLong() != rhs._messageId.toULongLong()) {
            return (lhs._messageId.toULongLong() < rhs._messageId.toULongLong());
        }
    }

    // Messages are the same - fall back to lexicographic comparison of the location
    return (lhs._properties._location.toString(false) < rhs._properties._location.toString(false));
}

static bool updateMessagesMetaData(ImapStrategyContextBase *context, 
                                   const QMailMessageKey &storedKey, 
                                   const QMailMessageKey &unseenKey, 
                                   const QMailMessageKey &seenKey,
                                   const QMailMessageKey &flaggedKey,
                                   const QMailMessageKey &unreadElsewhereKey,
                                   const QMailMessageKey &importantElsewhereKey,
                                   const QMailMessageKey &unavailableKey,
                                   const QMailMessageKey &movedKey)
{
    bool result = true;
    QMailMessageKey reportedKey(seenKey | unseenKey);
    QMailMessageKey unflaggedKey(reportedKey & ~flaggedKey);

    // Mark as deleted any messages that the server does not report
    QMailMessageKey nonexistentKey(storedKey & ~reportedKey & ~movedKey);
    QMailMessageIdList ids(QMailStore::instance()->queryMessages(nonexistentKey));
    
    if (!QMailStore::instance()->updateMessagesMetaData(nonexistentKey, QMailMessage::Removed, true)) {
        result = false;
        qWarning() << "Unable to update removed message metadata for account:" << context->config().id();
    }
    
    // Restore any messages thought to be unavailable that the server now reports
    QMailMessageKey reexistentKey(unavailableKey & reportedKey);
    if (!QMailStore::instance()->updateMessagesMetaData(reexistentKey, QMailMessage::Removed, false)) {
        result = false;
        qWarning() << "Unable to update un-removed message metadata for account:" << context->config().id();
    }

    foreach (const QMailMessageMetaData& r, QMailStore::instance()->messagesMetaData(nonexistentKey, QMailMessageKey::ServerUid))  {
        const QString &uid(r.serverUid()); 
        // We might have a deletion record for this UID
        if (!QMailStore::instance()->purgeMessageRemovalRecords(context->config().id(), QStringList() << uid)) {
            result = false;
            qWarning() << "Unable to purge message records for account:" << context->config().id();
        }
        context->completedMessageAction(uid);
    }

    // Update any messages that are reported as read elsewhere, that previously were not read elsewhere
    if (!QMailStore::instance()->updateMessagesMetaData(seenKey & unreadElsewhereKey, QMailMessage::Read, true)
        || !QMailStore::instance()->updateMessagesMetaData(seenKey & unreadElsewhereKey, QMailMessage::ReadElsewhere, true)) {
        result = false;
        qWarning() << "Unable to update read message metadata for account:" << context->config().id();
    }

    // Update any messages that are reported as unread elsewhere, that previously were read elsewhere
    if (!QMailStore::instance()->updateMessagesMetaData(unseenKey & ~unreadElsewhereKey, QMailMessage::Read, false)
        || !QMailStore::instance()->updateMessagesMetaData(unseenKey & ~unreadElsewhereKey, QMailMessage::ReadElsewhere, false)) {
        result = false;
        qWarning() << "Unable to update unread message metadata for account:" << context->config().id();
    }
    
    // Update any messages that are reported as important elsewhere, that previously were not important elsewhere
    if (!QMailStore::instance()->updateMessagesMetaData(flaggedKey & ~importantElsewhereKey, QMailMessage::Important, true)
        || !QMailStore::instance()->updateMessagesMetaData(flaggedKey & ~importantElsewhereKey, QMailMessage::ImportantElsewhere, true)) {
        result = false;
        qWarning() << "Unable to update important status flag message metadata for account:" << context->config().id();
    }

    // Update any messages that are reported as not important elsewhere, that previously were important elsewhere
    if (!QMailStore::instance()->updateMessagesMetaData(unflaggedKey & importantElsewhereKey, QMailMessage::Important, false)
        || !QMailStore::instance()->updateMessagesMetaData(unflaggedKey & importantElsewhereKey, QMailMessage::ImportantElsewhere, false)) {
        result = false;
        qWarning() << "Unable to update not important status flag message metadata for account:" << context->config().id();
    }
    
    return result;
}

static bool findFetchContinuationStart(const QMailMessage &message, const QMailMessagePart::Location &location, int *start)
{
    if (message.id().isValid()) {
        *start = 0;
        if (location.isValid() && message.contains(location)) {
            const QMailMessagePart &part(message.partAt(location));
            if (part.hasBody()) {
                *start = part.body().length();
            }
        } else {
            if (message.hasBody()) {
                *start = message.body().length();
            }
        }
        return true;
    }

    return false;
}

static QString numericUidSequence(const QStringList &uids)
{
    QStringList numericUids;
    foreach (const QString &uid, uids) {
        numericUids.append(ImapProtocol::uid(uid));
    }

    return IntegerRegion(numericUids).toString();
}

static bool transferPartBodies(QMailMessagePartContainer &destination, const QMailMessagePartContainer &source)
{
    if (destination.partCount() != source.partCount()) {
        qWarning() << "transferPartBodies detected copy failure, aborting transfer. Part count, destination" << destination.partCount() << "source" << source.partCount();
        return false; // copy failed, abort entire move
    }

    if (source.hasBody()) {
        destination.setBody(source.body());
    } else if (source.partCount() > 0) {
        for (uint i = 0; i < source.partCount(); ++i) {
            const QMailMessagePart &sourcePart = source.partAt(i);
            QMailMessagePart &destinationPart = destination.partAt(i);

            return transferPartBodies(destinationPart, sourcePart);
        }
    }
    return true;
}

static bool transferMessageData(QMailMessage &message, const QMailMessage &source)
{
    // TODO: this whole concept is wrong - we might lose data by replacing the original message...

    if (!transferPartBodies(message, source))
        return false;

    if (!message.customField("qtopiamail-detached-filename").isEmpty()) {
        // We have modified the content, so the detached file data is no longer sufficient
        message.removeCustomField("qtopiamail-detached-filename");
    }

    if (source.status() & QMailMessage::ContentAvailable) {
        message.setStatus(QMailMessage::ContentAvailable, true);
    }
    if (source.status() & QMailMessage::PartialContentAvailable) {
        message.setStatus(QMailMessage::PartialContentAvailable, true);
    }
    return true;
}


ImapClient *ImapStrategyContextBase::client() 
{ 
    return _client; 
}

ImapProtocol &ImapStrategyContextBase::protocol() 
{ 
    return _client->_protocol; 
}

const ImapMailboxProperties &ImapStrategyContextBase::mailbox() 
{ 
    return _client->_protocol.mailbox(); 
}

const QMailAccountConfiguration &ImapStrategyContextBase::config() 
{ 
    return _client->_config; 
}

void ImapStrategyContextBase::updateStatus(const QString &text) 
{ 
    emit _client->updateStatus(text);
}

void ImapStrategyContextBase::progressChanged(uint progress, uint total) 
{ 
    emit _client->progressChanged(progress, total);
}

void ImapStrategyContextBase::completedMessageAction(const QString &text) 
{ 
    emit _client->messageActionCompleted(text);
}

void ImapStrategyContextBase::completedMessageCopy(QMailMessage &message, const QMailMessage &original) 
{ 
    emit _client->messageCopyCompleted(message, original);
}

void ImapStrategyContextBase::operationCompleted()
{ 
    // Update the status on any folders we modified
    foreach (const QMailFolderId &folderId, _modifiedFolders) {
        QMailFolder folder(folderId);
        _client->updateFolderCountStatus(&folder);

        if (!QMailStore::instance()->updateFolder(&folder)) {
            qWarning() << "Unable to update folder for account:" << _client->_config.id();
        }
    }

    _client->retrieveOperationCompleted();
}

void ImapStrategyContextBase::matchingMessageIds(const QMailMessageIdList &msgs)
{
    emit _client->matchingMessageIds(msgs);
}

/* A basic strategy to achieve an authenticated state with the server,
   and to provide default responses to IMAP command completion notifications,
*/
void ImapStrategy::newConnection(ImapStrategyContextBase *context)
{
    _transferState = Init;

    ImapConfiguration imapCfg(context->config());
    _baseFolder = imapCfg.baseFolder();

    initialAction(context);
}

void ImapStrategy::initialAction(ImapStrategyContextBase *context)
{
    if (context->protocol().loggingOut()) {
        context->protocol().close();
    }

    if (context->protocol().inUse()) {
        // We have effectively just completed authenticating
        transition(context, IMAP_Login, OpOk);
    } else {
        ImapConfiguration imapCfg(context->config());
        context->protocol().open(imapCfg);
    }
}

void ImapStrategy::mailboxListed(ImapStrategyContextBase *c, QMailFolder& folder, const QString &flags)
{
    if (!folder.id().isValid()) {
        // Only folders beneath the base folder are relevant
        QString path(folder.path());

        if (_baseFolder.isEmpty() || 
            (path.startsWith(_baseFolder, Qt::CaseInsensitive) && (path.length() == _baseFolder.length())) ||
            (path.startsWith(_baseFolder + c->protocol().delimiter(), Qt::CaseInsensitive))) {
            if (!QMailStore::instance()->addFolder(&folder)) {
                _error = true;
                qWarning() << "Unable to add folder for account:" << folder.parentAccountId() << "path:" << folder.path();
            }
        }
    }

    Q_UNUSED(flags)
}

void ImapStrategy::messageFetched(ImapStrategyContextBase *context, QMailMessage &message)
{
    // Store this message to the mail store
    if (message.id().isValid()) {
        if (!QMailStore::instance()->updateMessage(&message)) {
            _error = true;
            qWarning() << "Unable to add message for account:" << message.parentAccountId() << "UID:" << message.serverUid();
            return;
        }
    } else {
        int matching = QMailStore::instance()->countMessages(QMailMessageKey::serverUid(message.serverUid()) & QMailMessageKey::parentAccountId(message.parentAccountId()));
        if (matching == 0) {
            if (!QMailStore::instance()->addMessage(&message)) {
                _error = true;
                qWarning() << "Unable to add message for account:" << message.parentAccountId() << "UID:" << message.serverUid();
                return;
            }
        } else {
            if(matching > 1) {
                qWarning() << "A duplicate message (which shouldn't exist) is being updated, meaning they now could be falling out of sync."
                        << "Account: " << message.parentAccountId() << "UID:" << message.serverUid();
            }
            message.setId(QMailStore::instance()->message(message.serverUid(), message.parentAccountId()).id());

            if (!QMailStore::instance()->updateMessage(&message)) {
                _error = true;
                qWarning() << "Unable to add message for account:" << message.parentAccountId() << "UID:" << message.serverUid();
                return;
            }
        }

        context->folderModified(message.parentFolderId());
    }

    context->completedMessageAction(message.serverUid());
}

void ImapStrategy::dataFetched(ImapStrategyContextBase *context, QMailMessage &message, const QString &uid, const QString &section)
{
    // Store the updated message
    if (!QMailStore::instance()->updateMessage(&message)) {
        _error = true;
        qWarning() << "Unable to update message for account:" << message.parentAccountId() << "UID:" << message.serverUid();
        return;
    }

    context->completedMessageAction(uid);
            
    Q_UNUSED(section)
}

void ImapStrategy::nonexistentUid(ImapStrategyContextBase *context, const QString &uid)
{
    // Mark this message as deleted
    QMailMessage message(uid, context->config().id());
    if (message.id().isValid()) {
        message.setStatus(QMailMessage::Removed, true);
        if (!QMailStore::instance()->updateMessage(&message)) {
            _error = true;
            qWarning() << "Unable to update nonexistent message for account:" << message.parentAccountId() << "UID:" << message.serverUid();
            return;
        }
    }

    context->completedMessageAction(uid);
}

void ImapStrategy::messageStored(ImapStrategyContextBase *context, const QString &uid)
{
    context->completedMessageAction(uid);
}

void ImapStrategy::messageCopied(ImapStrategyContextBase *context, const QString &copiedUid, const QString &createdUid)
{
    // A copy operation should be reported as completed by the subsequent fetch, not the copy itself
    //context->completedMessageAction(copiedUid);

    Q_UNUSED(context)
    Q_UNUSED(copiedUid)
    Q_UNUSED(createdUid)
}

void ImapStrategy::messageCreated(ImapStrategyContextBase *context, const QMailMessageId &id, const QString &uid)
{
    Q_UNUSED(context)
    Q_UNUSED(id)
    Q_UNUSED(uid)
}

void ImapStrategy::downloadSize(ImapStrategyContextBase *context, const QString &uid, int length)
{
    Q_UNUSED(context)
    Q_UNUSED(uid)
    Q_UNUSED(length)
}

void ImapStrategy::urlAuthorized(ImapStrategyContextBase *context, const QString &url)
{
    Q_UNUSED(context)
    Q_UNUSED(url)
}

void ImapStrategy::folderCreated(ImapStrategyContextBase *context, const QString &folder)
{
    Q_UNUSED(context)
    Q_UNUSED(folder)
}

void ImapStrategy::folderDeleted(ImapStrategyContextBase *context, const QMailFolder &folder)
{
    Q_UNUSED(context)
    Q_UNUSED(folder)
}

void ImapStrategy::folderRenamed(ImapStrategyContextBase *context, const QMailFolder &folder, const QString &newPath)
{
    Q_UNUSED(context)
    Q_UNUSED(folder)
    Q_UNUSED(newPath)
}

/* A strategy to create a folder */
void ImapCreateFolderStrategy::transition(ImapStrategyContextBase* context, const ImapCommand cmd, const OperationStatus op)
{
    if(op != OpOk)
        qWarning() << "IMAP Response to cmd:" << cmd << " is not ok: " << op;

    switch(cmd)
    {
    case IMAP_Login:
        handleLogin(context);
        break;
    case IMAP_Create:
        handleCreate(context);
        break;
    default:
        qWarning() << "Unhandled IMAP response:" << cmd;
    }
}

void ImapCreateFolderStrategy::createFolder(const QMailFolderId &folderParent, const QString &name)
{
    _folders.append(qMakePair(folderParent, name));
}

void ImapCreateFolderStrategy::handleLogin(ImapStrategyContextBase *context)
{
    process(context);
}

void ImapCreateFolderStrategy::handleCreate(ImapStrategyContextBase *context)
{
    process(context);
}

void ImapCreateFolderStrategy::process(ImapStrategyContextBase *context)
{
    while(_folders.count() > 0) {
        QPair<QMailFolderId, QString> folder = _folders.takeFirst();
        context->protocol().sendCreate(folder.first, folder.second);
        _inProgress++;
    }
}

void ImapCreateFolderStrategy::folderCreated(ImapStrategyContextBase *context, const QString &folder)
{
    if(--_inProgress == 0)
        context->operationCompleted();

    Q_UNUSED(folder)
}


/* A strategy to delete a folder */
void ImapDeleteFolderStrategy::transition(ImapStrategyContextBase* context, const ImapCommand cmd, const OperationStatus op)
{
    if(op != OpOk)
        qWarning() << "IMAP Response to cmd:" << cmd << " is not ok: " << op;

    switch(cmd)
    {
    case IMAP_Login:
        handleLogin(context);
        break;
    case IMAP_Delete:
        handleDelete(context);
        break;
    default:
        qWarning() << "Unhandled IMAP response:" << cmd;
    }
}

void ImapDeleteFolderStrategy::deleteFolder(const QMailFolderId& folderId)
{
    _folderIds.append(folderId);
}

void ImapDeleteFolderStrategy::handleLogin(ImapStrategyContextBase* context)
{
    process(context);
}

void ImapDeleteFolderStrategy::handleDelete(ImapStrategyContextBase *context)
{
    process(context);
}

void ImapDeleteFolderStrategy::process(ImapStrategyContextBase *context)
{
    while(_folderIds.count() > 0) {
        deleteFolder(_folderIds.takeFirst(), context);
    }
}

void ImapDeleteFolderStrategy::deleteFolder(const QMailFolderId &folderId, ImapStrategyContextBase *context)
{
    QMailFolderKey childKey(QMailFolderKey::parentFolderId(folderId));
    QMailFolderIdList childrenList = QMailStore::instance()->queryFolders(childKey);

    foreach(QMailFolderId fid, childrenList) {
        deleteFolder(fid, context);
    }

    //now the parent is safe to delete
    context->protocol().sendDelete(QMailFolder(folderId));
    _inProgress++;
}

void ImapDeleteFolderStrategy::folderDeleted(ImapStrategyContextBase *context, const QMailFolder &folder)
{
    if(!QMailStore::instance()->removeFolder(folder.id()))
        qWarning() << "Unable to remove folder id: " << folder.id();

    if(--_inProgress == 0)
       context->operationCompleted();
}

/* A strategy to rename a folder */
void ImapRenameFolderStrategy::transition(ImapStrategyContextBase* context, const ImapCommand cmd, const OperationStatus op)
{
    if(op != OpOk)
        qWarning() << "IMAP Response to cmd:" << cmd << " is not ok: " << op;

    switch(cmd)
    {
    case IMAP_Login:
        handleLogin(context);
        break;
    case IMAP_Rename:
        handleRename(context);
        break;
    default:
        qWarning() << "Unhandled IMAP response:" << cmd;
    }
}

void ImapRenameFolderStrategy::renameFolder(const QMailFolderId &folderId, const QString &newName)
{
    _folderNewNames.append(qMakePair(folderId, newName));
}

void ImapRenameFolderStrategy::handleLogin(ImapStrategyContextBase *context)
{
    process(context);
}

void ImapRenameFolderStrategy::handleRename(ImapStrategyContextBase *context)
{
    process(context);
}

void ImapRenameFolderStrategy::process(ImapStrategyContextBase *context)
{
    while(_folderNewNames.count() > 0) {
        const QPair<QMailFolderId, QString> &folderId_name =  _folderNewNames.takeFirst();
        context->protocol().sendRename(QMailFolder(folderId_name.first), folderId_name.second);
        _inProgress++;
    }
}

void ImapRenameFolderStrategy::folderRenamed(ImapStrategyContextBase *context, const QMailFolder &folder, const QString &newPath)
{
    QString name;

    if(!context->protocol().delimiter().isNull()) {
        //only update if we're dealing with a hierachical system
        QChar delimiter = context->protocol().delimiter();
        if(folder.path().count(delimiter) == 0) {
            name = newPath;
        } else {
            name = newPath.section(delimiter, -1, -1);

            QMailFolderKey affectedFolderKey(QMailFolderKey::ancestorFolderIds(folder.id()));
            QMailFolderIdList affectedFolders = QMailStore::instance()->queryFolders(affectedFolderKey);

            while(!affectedFolders.isEmpty()) {
                QMailFolder childFolder(affectedFolders.takeFirst());
                QString path = childFolder.path();
                path.replace(0, folder.path().length(), newPath);
                childFolder.setPath(path);
                if(!QMailStore::instance()->updateFolder(&childFolder))
                    qWarning() << "Unable to locally change path of a subfolder";
            }
        }
    } else {
        name = newPath;
    }

    QMailFolder newFolder = folder;
    newFolder.setPath(newPath);
    newFolder.setDisplayName(name);

    if(!QMailStore::instance()->updateFolder(&newFolder))
        qWarning() << "Unable to locally rename folder";
    if(--_inProgress == 0)
        context->operationCompleted();
}

/* A strategy to traverse a list of messages, preparing each one for transmission
   by generating a URLAUTH token.
*/
void ImapPrepareMessagesStrategy::setUnresolved(const QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > &ids, bool external)
{
    _locations = ids;
    _external = external;
}

void ImapPrepareMessagesStrategy::newConnection(ImapStrategyContextBase *context)
{
    if (!_external) {
        // We only need internal references - generate IMAP URLs for each location
        while (!_locations.isEmpty()) {
            const QPair<QMailMessagePart::Location, QMailMessagePart::Location> &pair(_locations.first());

            QString url(ImapProtocol::url(pair.first, false, true));
            urlAuthorized(context, url);

            _locations.removeFirst();
        }

        context->operationCompleted();
        return;
    }

    ImapStrategy::newConnection(context);
}

void ImapPrepareMessagesStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus)
{
    switch( command ) {
        case IMAP_Login:
        {
            handleLogin(context);
            break;
        }
        
        case IMAP_GenUrlAuth:
        {
            handleGenUrlAuth(context);
            break;
        }
        
        case IMAP_Logout:
        {
            break;
        }
        
        default:
        {
            _error = true;
            qWarning() << "Unhandled IMAP response:" << command;
            break;
        }
    }
}

void ImapPrepareMessagesStrategy::handleLogin(ImapStrategyContextBase *context)
{
    nextMessageAction(context);
}

void ImapPrepareMessagesStrategy::handleGenUrlAuth(ImapStrategyContextBase *context)
{
    // We're finished with the previous location
    _locations.removeFirst();

    nextMessageAction(context);
}

void ImapPrepareMessagesStrategy::nextMessageAction(ImapStrategyContextBase *context)
{
    if (!_locations.isEmpty()) {
        const QPair<QMailMessagePart::Location, QMailMessagePart::Location> &pair(_locations.first());

        // Generate an authorized URL for this message content
        bool bodyOnly(false);
        if (!pair.first.isValid(false)) {
            // This is a full-message reference - for a single-part message, we should forward
            // only the body text; for multi-part we want the whole message
            QMailMessage message(pair.first.containingMessageId());
            if (message.multipartType() == QMailMessage::MultipartNone) {
                bodyOnly = true;
            }
        }
        context->protocol().sendGenUrlAuth(pair.first, bodyOnly);
    } else {
        messageListCompleted(context);
    }
}

void ImapPrepareMessagesStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    context->operationCompleted();
}

namespace {

struct ReferenceDetector
{
    bool operator()(const QMailMessagePart &part)
    {
        // Return false if there is an unresolved reference to stop traversal
        return ((part.referenceType() == QMailMessagePart::None) || !part.referenceResolution().isEmpty());
    }
};

bool hasUnresolvedReferences(const QMailMessage &message)
{
    // If foreachPart yields false there is at least one unresolved reference
    return (message.foreachPart(ReferenceDetector()) == false);
}

}

void ImapPrepareMessagesStrategy::urlAuthorized(ImapStrategyContextBase *, const QString &url)
{
    const QPair<QMailMessagePart::Location, QMailMessagePart::Location> &pair(_locations.first());

    // We now have an authorized URL for this location
    QMailMessageId referringId(pair.second.containingMessageId());
    if (referringId.isValid()) {
        QMailMessage referer(referringId);
        if (referer.contains(pair.second)) {
            QMailMessagePart &part(referer.partAt(pair.second));

            part.setReferenceResolution(url);

            // Have we resolved all references in this message?
            if (!hasUnresolvedReferences(referer)) {
                referer.setStatus(QMailMessage::HasUnresolvedReferences, false);
            }

            if (!QMailStore::instance()->updateMessage(&referer)) {
                _error = true;
                qWarning() << "Unable to update message for account:" << referer.parentAccountId();
            }
        } else {
            _error = true;
            qWarning() << "Unable to resolve reference to invalid part:" << pair.second.toString(false);
        }
    } else {
        // Update this message with its own location reference
        QMailMessage referencedMessage(pair.first.containingMessageId());
        referencedMessage.setExternalLocationReference(url);

        if (!QMailStore::instance()->updateMessage(&referencedMessage)) {
            _error = true;
            qWarning() << "Unable to update message for account:" << referencedMessage.parentAccountId();
        }
    }
}


/* A strategy that provides an interface for defining a set of messages 
   or message parts to operate on, and an abstract interface messageListMessageAction() 
   for operating on messages.
   
   Also implements logic to determine which messages or message part to operate 
   on next.
*/
void ImapMessageListStrategy::clearSelection()
{
    _selectionMap.clear();
    _folderItr = _selectionMap.end();
}

void ImapMessageListStrategy::selectedMailsAppend(const QMailMessageIdList& ids) 
{
    if (ids.count() == 0)
        return;

    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentFolderId | QMailMessageKey::PreviousParentFolderId | QMailMessageKey::ServerUid);
    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(ids), props)) {
        uint serverUid(stripFolderPrefix(metaData.serverUid()).toUInt());
        _selectionMap[serverFolderId(metaData)].append(MessageSelector(serverUid, metaData.id(), SectionProperties()));
    }
}

void ImapMessageListStrategy::selectedSectionsAppend(const QMailMessagePart::Location &location)
{
    QMailMessageMetaData metaData(location.containingMessageId());
    if (metaData.id().isValid()) {
        uint serverUid(stripFolderPrefix(metaData.serverUid()).toUInt());
        _selectionMap[serverFolderId(metaData)].append(MessageSelector(serverUid, metaData.id(), SectionProperties(location)));
    }
}

void ImapMessageListStrategy::newConnection(ImapStrategyContextBase *context)
{
    setCurrentMailbox(QMailFolderId());

    ImapStrategy::newConnection(context);
}

void ImapMessageListStrategy::initialAction(ImapStrategyContextBase *context)
{
    resetMessageListTraversal();

    ImapStrategy::initialAction(context);
}

void ImapMessageListStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus)
{
    switch( command ) {
        case IMAP_Login:
        {
            handleLogin(context);
            break;
        }
        
        case IMAP_Select:
        {
            handleSelect(context);
            break;
        }
        
        case IMAP_Create:
        {
            handleCreate(context);
            break;
        }
        
        case IMAP_Delete:
        {
            handleDelete(context);
            break;
        }
        
        case IMAP_Rename:
        {
            handleRename(context);
            break;
        }
        
        case IMAP_Close:
        {
            handleClose(context);
            break;
        }
        
        case IMAP_Logout:
        {
            break;
        }
        
        default:
        {
            _error = true;
            qWarning() << "Unhandled IMAP response:" << command;
            break;
        }
    }
}

void ImapMessageListStrategy::handleLogin(ImapStrategyContextBase *context)
{
    messageListMessageAction(context);
}

void ImapMessageListStrategy::handleSelect(ImapStrategyContextBase *context)
{
    // We're completing a message or section
    messageListMessageAction(context);
}

void ImapMessageListStrategy::handleCreate(ImapStrategyContextBase *context)
{
    messageListMessageAction(context);
}

void ImapMessageListStrategy::handleDelete(ImapStrategyContextBase *context)
{
    messageListMessageAction(context);
}

void ImapMessageListStrategy::handleRename(ImapStrategyContextBase *context)
{
    messageListMessageAction(context);
}


void ImapMessageListStrategy::handleClose(ImapStrategyContextBase *context)
{
    messageListMessageAction(context);
}

void ImapMessageListStrategy::resetMessageListTraversal()
{
    _folderItr = _selectionMap.begin();
    if (_folderItr != _selectionMap.end()) {
        FolderSelections &folder(*_folderItr);
        qSort(folder.begin(), folder.end(), messageSelectorLessThan);

        _selectionItr = folder.begin();
    }
}

bool ImapMessageListStrategy::selectNextMessageSequence(ImapStrategyContextBase *context, int maximum, bool folderActionPermitted)
{
    _messageUids.clear();
    _msgSection = QMailMessagePart::Location();
    _sectionStart = 0;
    _sectionEnd = SectionProperties::All;

    if (!folderActionPermitted) {
        if (messageListFolderActionRequired()) {
            return false;
        }
    }

    if (_folderItr == _selectionMap.end()) {
        messageListCompleted(context);
        return false;
    }
        
    FolderSelections::ConstIterator selectionEnd = _folderItr.value().end();
    while (_selectionItr == selectionEnd) {
        ++_folderItr;
        if (_folderItr == _selectionMap.end())
            break;

        FolderSelections &folder(*_folderItr);
        qSort(folder.begin(), folder.end(), messageSelectorLessThan);

        _selectionItr = folder.begin();
        selectionEnd = folder.end();
    }

    if ((_folderItr == _selectionMap.end()) || !_folderItr.key().isValid()) {
        setCurrentMailbox(QMailFolderId());
        _selectionMap.clear();
        messageListFolderAction(context);
        return false;
    }

    _transferState = Complete;

    QMailFolderId mailboxId = _folderItr.key();
    if (mailboxId != _currentMailbox.id()) {
        setCurrentMailbox(mailboxId);
        messageListFolderAction(context);
        return false;
    }

    QString mailboxIdStr = QString::number(mailboxId.toULongLong()) + UID_SEPARATOR;

    // Get consecutive full messages
    while ((_messageUids.count() < maximum) &&
           (_selectionItr != selectionEnd) &&
           (_selectionItr->_properties.isEmpty())) {
        _messageUids.append((*_selectionItr).uidString(mailboxIdStr));
        ++_selectionItr;
    }

    if (!_messageUids.isEmpty()) {
        return true;
    }

    // TODO: Get multiple parts for a single message in one roundtrip?
    if (_messageUids.isEmpty() && (_selectionItr != selectionEnd)) {
        const MessageSelector &selector(*_selectionItr);
        ++_selectionItr;

        _messageUids.append(selector.uidString(mailboxIdStr));
        _msgSection = selector._properties._location;
        
        if (selector._properties._minimum != SectionProperties::All) {
            _sectionEnd = (selector._properties._minimum - 1);

            bool valid(_sectionEnd != SectionProperties::All);
            if (valid) {
                // Find where we should continue fetching from
                QMailMessage message(_messageUids.first(), context->config().id());
                valid = findFetchContinuationStart(message, _msgSection, &_sectionStart);
                
                // Try to avoid sending bad IMAP commands even when the server gives bogus values
                if (_sectionStart >= _sectionEnd) {
                    qWarning() << "Invalid message section range" 
                               << "account:" << message.parentAccountId() 
                               << "UID:" << message.serverUid() 
                               << "start:" << _sectionStart 
                               << "end:" << _sectionEnd;
                    valid = false;
                }
            }

            if (!valid) {
                if (_sectionStart <= _sectionEnd) {
                    qMailLog(Messaging) << "Could not complete part: invalid location or invalid uid";
                } else {
                    // already have retrieved minimum bytes, so nothing to do
                }

                // Try the next list element
                return selectNextMessageSequence(context, maximum, folderActionPermitted);
            }
        }
    }

    return !_messageUids.isEmpty();
}

bool ImapMessageListStrategy::messageListFolderActionRequired()
{
    return ((_folderItr == _selectionMap.end()) || (_selectionItr == _folderItr.value().end()));
}

void ImapMessageListStrategy::messageListFolderAction(ImapStrategyContextBase *context)
{
    if (_currentMailbox.id().isValid()) {
        if (_currentMailbox.id() == context->mailbox().id) {
            // We already have the appropriate mailbox selected
            handleSelect(context);
        } else if (_currentMailbox.id() == QMailFolder::LocalStorageFolderId) {
            // No folder should be selected
            context->protocol().sendClose();
        } else {
            context->protocol().sendSelect(_currentMailbox);
        }
    } else {
        messageListCompleted(context);
    }
}

void ImapMessageListStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    context->operationCompleted();
}

void ImapMessageListStrategy::setCurrentMailbox(const QMailFolderId &id)
{
    if (id.isValid()) { 
        _currentMailbox = QMailFolder(id);

        // Store the current modification sequence value for this folder, if we have one
        _currentModSeq = _currentMailbox.customField("qmf-highestmodseq");
    } else {
        _currentMailbox = QMailFolder();
        _currentModSeq = QString();
    }
}


/* A strategy which provides an interface for defining a set of messages
   or message parts to retrieve, and logic to retrieve those messages
   or message parts and emit progress notifications.
*/
void ImapFetchSelectedMessagesStrategy::clearSelection()
{
    ImapMessageListStrategy::clearSelection();
    _totalRetrievalSize = 0;
    _listSize = 0;
    _retrievalSize.clear();
}

void ImapFetchSelectedMessagesStrategy::setOperation(QMailRetrievalAction::RetrievalSpecification spec)
{
    if (spec == QMailRetrievalAction::MetaData) {
        _headerLimit = UINT_MAX;
    } else {
        _headerLimit = 0;
    }
}

void ImapFetchSelectedMessagesStrategy::selectedMailsAppend(const QMailMessageIdList& ids) 
{
    _listSize += ids.count();
    if (_listSize == 0)
        return;

    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentFolderId | QMailMessageKey::PreviousParentFolderId | QMailMessageKey::ServerUid | QMailMessageKey::Size);

    // Break retrieval of message meta data into chunks to reduce peak memory use
    QMailMessageIdList idsBatch;
    const int batchSize(100);
    int i = 0;
    while (i < ids.count()) {
        idsBatch.clear();
        while ((i < ids.count()) && (idsBatch.count() < batchSize)) {
            idsBatch.append(ids[i]);
            ++i;
        }

        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(idsBatch), props)) {
            uint serverUid(stripFolderPrefix(metaData.serverUid()).toUInt());

            //use previousparentfolderid if available since the message may have been moved locally
            QMailFolderId remoteFolderId(serverFolderId(metaData));

            _selectionMap[remoteFolderId].append(MessageSelector(serverUid, metaData.id(), SectionProperties()));

            uint size = metaData.indicativeSize();
            uint bytes = metaData.size();

            _retrievalSize.insert(metaData.serverUid(), qMakePair(qMakePair(size, bytes), 0u));
            _totalRetrievalSize += size;
        }
    }

    // Report the total size we will retrieve
    _progressRetrievalSize = 0;
}

void ImapFetchSelectedMessagesStrategy::selectedSectionsAppend(const QMailMessagePart::Location &location, int minimum)
{
    _listSize += 1;

    const QMailMessage message(location.containingMessageId());
    if (message.id().isValid()) {
        uint serverUid(stripFolderPrefix(message.serverUid()).toUInt());
        _selectionMap[serverFolderId(message)].append(MessageSelector(serverUid, message.id(), SectionProperties(location, minimum)));

        uint size = 0;
        uint bytes = minimum;

        if (minimum > 0) {
            size = 1;
        } else if (location.isValid() && message.contains(location)) {
            // Find the size of this part
            const QMailMessagePart &part(message.partAt(location));
            size = part.indicativeSize();
            bytes = part.contentDisposition().size();
        }
        // Required to show progress when downloading attachments
        if (size < 1)
            size = bytes/1024;

        _retrievalSize.insert(message.serverUid(), qMakePair(qMakePair(size, bytes), 0u));
        _totalRetrievalSize += size;
    }
}

void ImapFetchSelectedMessagesStrategy::newConnection(ImapStrategyContextBase *context)
{
    _progressRetrievalSize = 0;
    _messageCount = 0;
    _outstandingFetches = 0;

    ImapConfiguration imapCfg(context->config());
    if (!imapCfg.isAutoDownload()) {
        _headerLimit = imapCfg.maxMailSize() * 1024;
    }

    ImapMessageListStrategy::newConnection(context);
}

void ImapFetchSelectedMessagesStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_UIDFetch:
        {
            handleUidFetch(context);
            break;
        }
        
        default:
        {
            ImapMessageListStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapFetchSelectedMessagesStrategy::handleLogin(ImapStrategyContextBase *context)
{
    if (_totalRetrievalSize)
        context->progressChanged(0, _totalRetrievalSize);

    messageListMessageAction(context);
}

void ImapFetchSelectedMessagesStrategy::handleUidFetch(ImapStrategyContextBase *context)
{
    --_outstandingFetches;
    if (_outstandingFetches == 0) {
        messageListMessageAction(context);
    }
}

void ImapFetchSelectedMessagesStrategy::messageListMessageAction(ImapStrategyContextBase *context)
{
    if (messageListFolderActionRequired()) {
        // We need to change folders
        selectNextMessageSequence(context, 1, true);
        return;
    }

    _messageCountIncremental = _messageCount;

    while (selectNextMessageSequence(context, DefaultBatchSize, false)) {
        _messageCount += _messageUids.count();

        QString msgSectionStr;
        if (_msgSection.isValid()) {
            msgSectionStr = _msgSection.toString(false);
        }

        if (_msgSection.isValid() || (_sectionEnd != SectionProperties::All)) {
            context->protocol().sendUidFetchSection(numericUidSequence(_messageUids), msgSectionStr, _sectionStart, _sectionEnd);
        } else {
            context->protocol().sendUidFetch(ContentFetchFlags, numericUidSequence(_messageUids));
        }

        ++_outstandingFetches;
    }
}

void ImapFetchSelectedMessagesStrategy::downloadSize(ImapStrategyContextBase *context, const QString &uid, int length)
{
    if (!uid.isEmpty()) {
        RetrievalMap::iterator it = _retrievalSize.find(uid);
        if (it != _retrievalSize.end()) {
            QPair<QPair<uint, uint>, uint> &values = it.value();

            // Calculate the percentage of the retrieval completed
            uint totalBytes = values.first.second;
            uint percentage = totalBytes ? qMin<uint>(length * 100 / totalBytes, 100) : 100;

            if (percentage > values.second) {
                values.second = percentage;

                // Update the progress figure to count the retrieved portion of this message
                uint partialSize = values.first.first * percentage / 100;
                context->progressChanged(_progressRetrievalSize + partialSize, _totalRetrievalSize);
            }
        }
    }
}

void ImapFetchSelectedMessagesStrategy::messageFetched(ImapStrategyContextBase *context, QMailMessage &message)
{ 
    ImapMessageListStrategy::messageFetched(context, message);

    itemFetched(context, message.serverUid());
}

void ImapFetchSelectedMessagesStrategy::dataFetched(ImapStrategyContextBase *context, QMailMessage &message, const QString &uid, const QString &section)
{ 
    ImapMessageListStrategy::dataFetched(context, message, uid, section);

    itemFetched(context, message.serverUid());
}

void ImapFetchSelectedMessagesStrategy::itemFetched(ImapStrategyContextBase *context, const QString &uid)
{ 
    RetrievalMap::iterator it = _retrievalSize.find(uid);
    if (it != _retrievalSize.end()) {
        // Update the progress figure
        _progressRetrievalSize += it.value().first.first;
        context->progressChanged(_progressRetrievalSize, _totalRetrievalSize);

        it = _retrievalSize.erase(it);
    }

    if (_listSize) {
        int count = qMin(++_messageCountIncremental + 1, _listSize);
        context->updateStatus(QObject::tr("Completing %1 / %2").arg(count).arg(_listSize));
    }
}
/* A strategy to search all folders */

void ImapSearchMessageStrategy::searchArguments(const QMailMessageKey &searchCriteria, const QString &bodyText, const QMailMessageSortKey &sort)
{
   SearchData search;
   search.criteria = searchCriteria;
   search.bodyText = bodyText;
   search.sort = sort;

    _searches.append(search);
    _canceled = false;
}

void ImapSearchMessageStrategy::cancelSearch()
{
    _searches.clear();
    _canceled = true;
}

void ImapSearchMessageStrategy::transition(ImapStrategyContextBase *c, ImapCommand cmd, OperationStatus status)
{
    switch(cmd) {
    case IMAP_Search_Message:
        handleSearchMessage(c);
        break;
    default:
        ImapRetrieveFolderListStrategy::transition(c, cmd, status);
    }
}

void ImapSearchMessageStrategy::folderListFolderAction(ImapStrategyContextBase *context)
{
    if(_canceled)
        return; //stop it searching

    //if there are messages, lets search it
    const ImapMailboxProperties &properties(context->mailbox());
    if(properties.exists > 0) {
        const SearchData &search = _searches.first();
        context->protocol().sendSearchMessages(search.criteria, search.bodyText, search.sort);
    } else {
        processNextFolder(context);
    }
}

void ImapSearchMessageStrategy::messageFetched(ImapStrategyContextBase *context, QMailMessage &message)
{
    if(_canceled)
        return;

    message.setStatus(QMailMessage::Temporary, true);
    ImapRetrieveFolderListStrategy::messageFetched(context, message);

    _fetchedList.append(message.id());
    Q_UNUSED(context)
}

void ImapSearchMessageStrategy::handleUidFetch(ImapStrategyContextBase *context)
{
    if(_canceled)
        return;

    context->matchingMessageIds(_fetchedList);
    _fetchedList.clear();

    processNextFolder(context);
}

void ImapSearchMessageStrategy::handleSearchMessage(ImapStrategyContextBase *context)
{
    if(_canceled)
        return;

    const ImapMailboxProperties &properties(context->mailbox());
    QList<QMailMessageId> searchResults;
    IntegerRegion uidsToFetch;

    foreach(const QString &uidString, properties.uidList) {
        int uid(stripFolderPrefix(uidString).toInt());
        if(QMailStore::instance()->countMessages(QMailMessageKey::serverUid(uidString) & QMailMessageKey::parentAccountId(context->config().id())) == 1)
            searchResults.append(QMailMessageId(uid));
        else
            uidsToFetch.add(uid);
    }

    if(!searchResults.isEmpty())
        context->matchingMessageIds(searchResults);

    if(uidsToFetch.isEmpty())
        processNextFolder(context);
    else
        context->protocol().sendUidFetch(MetaDataFetchFlags, uidsToFetch.toString());
}

void ImapSearchMessageStrategy::messageListCompleted(ImapStrategyContextBase *context)
 {
    if(_canceled)
        return;
     _searches.removeFirst();
     context->operationCompleted();
 }

/* A strategy that provides an interface for processing a set of folders.
*/
void ImapFolderListStrategy::clearSelection()
{
    ImapFetchSelectedMessagesStrategy::clearSelection();
    _mailboxIds.clear();
}

void ImapFolderListStrategy::selectedFoldersAppend(const QMailFolderIdList& ids) 
{
    _mailboxIds += ids;
    _processable += ids.count();
}

void ImapFolderListStrategy::newConnection(ImapStrategyContextBase *context)
{
    _folderStatus.clear();

    ImapFetchSelectedMessagesStrategy::newConnection(context);
}

void ImapFolderListStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_List:
        {
            handleList(context);
            break;
        }
        
        case IMAP_Search:
        {
            handleSearch(context);
            break;
        }
        
        default:
        {
            ImapFetchSelectedMessagesStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapFolderListStrategy::handleLogin(ImapStrategyContextBase *context)
{
    _transferState = List;

    processNextFolder(context);
}

void ImapFolderListStrategy::handleList(ImapStrategyContextBase *context)
{
    if (!_currentMailbox.id().isValid()) {
        // Try to proceed to another mailbox
        processNextFolder(context);
    } else {
        // If the current folder is not yet selected
        if (_currentMailbox.id() != context->mailbox().id) {
            if (_folderStatus.contains(_currentMailbox.id())) {
                FolderStatus folderState = _folderStatus[_currentMailbox.id()];
                if (folderState & NoSelect) {
                    // We can't select this folder
                    processNextFolder(context);
                } else {
                    // Select this folder
                    context->protocol().sendSelect( _currentMailbox );
                    return;
                }
            } else {
                // This folder does not exist, according to the listing...
                processNextFolder(context);
            }
        } else {
            // This mailbox is selected
            folderListFolderAction(context);
        }
    }
}

void ImapFolderListStrategy::handleSelect(ImapStrategyContextBase *context)
{
    if (_transferState == List) {
        // We have selected this folder - find out how many undiscovered messages exist
        const ImapMailboxProperties &properties(context->mailbox());

        if (properties.exists == 0) {
            // No messages - nothing to discover...
        } else {
            // If CONDSTORE is available, we may know that no changes have occurred
            if (!properties.noModSeq && (properties.highestModSeq == _currentModSeq)) {
                // No changes have occurred - bypass searching entirely
            } else {
                // Find the UID of the most recent message we have previously discovered in this folder
                QMailFolder folder(properties.id);
                quint32 clientMaxUid(folder.customField("qmf-max-serveruid").toUInt());

                if (clientMaxUid == 0) {
                    // We don't have any messages for this folder - fall through to handleSearch,
                    // where undiscoveredCount will be set to properties.exists
                } else {
                    if (properties.uidNext <= (clientMaxUid + 1)) {
                        // There's no need to search because we already have the highest UID
                    } else {
                        // Start our search above the current highest UID
                        context->protocol().sendSearch(0, QString("UID %1:%2").arg(clientMaxUid + 1).arg(properties.uidNext));
                        return;
                    }
                }
            }
        }

        handleSearch(context);
    } else {
        ImapMessageListStrategy::handleSelect(context);
    }
}

void ImapFolderListStrategy::handleSearch(ImapStrategyContextBase *context)
{
    updateUndiscoveredCount(context);

    // We have finished with this folder
    folderListFolderAction(context);
}

void ImapFolderListStrategy::folderListFolderAction(ImapStrategyContextBase *context)
{
    // The current mailbox is now selected - subclasses clients should do something now

    // Go onto the next mailbox
    processNextFolder(context);
}

void ImapFolderListStrategy::processNextFolder(ImapStrategyContextBase *context)
{
    if (nextFolder()) {
        processFolder(context);
        return;
    }

    folderListCompleted(context);
}

bool ImapFolderListStrategy::nextFolder()
{
    while (!_mailboxIds.isEmpty()) {
        QMailFolderId folderId(_mailboxIds.takeFirst());

        // Process this folder
        setCurrentMailbox(folderId);

        // Bypass any folder for which synchronization is disabled
        if (_currentMailbox.status() & QMailFolder::SynchronizationEnabled)
            return true;
    }

    return false;
}

void ImapFolderListStrategy::processFolder(ImapStrategyContextBase *context)
{
    QMailFolderId folderId = _currentMailbox.id();
    if(_folderStatus.contains(folderId) && _folderStatus[folderId] & NoSelect)
        context->protocol().sendList(_currentMailbox, "%");
    else
        context->protocol().sendSelect(_currentMailbox);

    context->progressChanged(++_processed, _processable);
}

void ImapFolderListStrategy::folderListCompleted(ImapStrategyContextBase *context)
{
    // We have retrieved all the folders - process any messages
    messageListMessageAction(context);
}

void ImapFolderListStrategy::mailboxListed(ImapStrategyContextBase *context, QMailFolder &folder, const QString &flags)
{
    ImapStrategy::mailboxListed(context, folder, flags);

    if (folder.id().isValid()) {
        // Record the status of the listed mailbox
        int status = 0;
        if (flags.indexOf("NoInferiors", 0, Qt::CaseInsensitive) != -1)
            status |= NoInferiors;
        if (flags.indexOf("NoSelect", 0, Qt::CaseInsensitive) != -1)
            status |= NoSelect;
        if (flags.indexOf("Marked", 0, Qt::CaseInsensitive) != -1)
            status |= Marked;
        if (flags.indexOf("Unmarked", 0, Qt::CaseInsensitive) != -1)
            status |= Unmarked;
        if (flags.indexOf("HasChildren", 0, Qt::CaseInsensitive) != -1)
            status |= HasChildren;
        if (flags.indexOf("HasNoChildren", 0, Qt::CaseInsensitive) != -1)
            status |= HasNoChildren;

        _folderStatus[folder.id()] = static_cast<FolderStatus>(status);
    }
}

void ImapFolderListStrategy::updateUndiscoveredCount(ImapStrategyContextBase *context)
{
    const ImapMailboxProperties &properties(context->mailbox());

    // Initial case set the undiscovered count to exists in the case of no 
    // max-serveruid set for the folder
    int undiscovered(properties.exists);

    QMailFolder folder(_currentMailbox.id());
    int clientMax(folder.customField("qmf-max-serveruid").toUInt());
    if (clientMax) {
        // The undiscovered count for a folder is the number of messages on the server newer 
        // than the most recent (highest server uid) message in the folder.
        undiscovered = properties.msnList.count();
    }
    
    if (uint(undiscovered) != folder.serverUndiscoveredCount()) {
        folder.setServerUndiscoveredCount(undiscovered);

        if (!QMailStore::instance()->updateFolder(&folder)) {
            _error = true;
            qWarning() << "Unable to update folder for account:" << context->config().id();
        }
    }
}


/* An abstract strategy. To be used as a base class for strategies that 
   iterate over mailboxes Previewing and Completing discovered mails.
*/
void ImapSynchronizeBaseStrategy::newConnection(ImapStrategyContextBase *context)
{
    _retrieveUids.clear();

    ImapFolderListStrategy::newConnection(context);
}

void ImapSynchronizeBaseStrategy::handleLogin(ImapStrategyContextBase *context)
{
    _completionList.clear();
    _completionSectionList.clear();

    ImapFolderListStrategy::handleLogin(context);
}

void ImapSynchronizeBaseStrategy::handleSelect(ImapStrategyContextBase *context)
{
    // We have selected the current mailbox
    if (_transferState == Preview) {
        // The scheduled fetch command was pipelined
    } else if (_transferState == Complete) {
        // We're completing a message or section
        messageListMessageAction(context);
    } else {
        ImapFolderListStrategy::handleSelect(context);
    }
}

void ImapSynchronizeBaseStrategy::handleUidFetch(ImapStrategyContextBase *context)
{
    if (_transferState == Preview) {    //getting headers
        --_outstandingPreviews;
        if (_outstandingPreviews == 0) {
            fetchNextMailPreview(context);
        }
    } else if (_transferState == Complete) {
        ImapFetchSelectedMessagesStrategy::handleUidFetch(context);
    }
}

void ImapSynchronizeBaseStrategy::processUidSearchResults(ImapStrategyContextBase *)
{
    _error = true;
    qWarning() << "ImapSynchronizeBaseStrategy::processUidSearchResults: Unexpected location!";
}

void ImapSynchronizeBaseStrategy::previewDiscoveredMessages(ImapStrategyContextBase *context)
{
    // Process our list of all messages to be retrieved for each mailbox
    _total = 0;
    QList<QPair<QMailFolderId, QStringList> >::const_iterator it = _retrieveUids.begin(), end = _retrieveUids.end();
    for ( ; it != end; ++it)
        _total += it->second.count();

    if (_total) {
        context->updateStatus(QObject::tr("Previewing", "Previewing <number of messages>") + QChar(' ') + QString::number(_total));
    }

    _progress = 0;
    context->progressChanged(_progress, _total);

    _transferState = Preview;

    if (!selectNextPreviewFolder(context)) {
        // Could be no mailbox has been selected to be stored locally
        messageListCompleted(context);
    }
}

bool ImapSynchronizeBaseStrategy::selectNextPreviewFolder(ImapStrategyContextBase *context)
{
    if (_retrieveUids.isEmpty()) {
        setCurrentMailbox(QMailFolderId());
        _newUids = QStringList();
        return false;
    }

    // In preview mode, select the mailboxes where retrievable messages are located
    QPair<QMailFolderId, QStringList> next = _retrieveUids.takeFirst();
    setCurrentMailbox(next.first);

    _newUids = next.second;
    _outstandingPreviews = 0;
    
    FolderStatus folderState = _folderStatus[_currentMailbox.id()];
    if (folderState & NoSelect) {
        // Bypass the select and UID search, and go directly to the search result handler
        processUidSearchResults(context);
    } else {
        if (_currentMailbox.id() == context->mailbox().id) {
            // We already have the appropriate mailbox selected
            fetchNextMailPreview(context);
        } else {
            if (_transferState == List) {
                QString status = QObject::tr("Checking", "Checking <mailbox name>") + QChar(' ') + _currentMailbox.displayName();
                context->updateStatus( status );
            }

            context->protocol().sendSelect( _currentMailbox );

            // Send fetch command without waiting for Select response
            fetchNextMailPreview(context);
        }
    }

    return true;
}

void ImapSynchronizeBaseStrategy::fetchNextMailPreview(ImapStrategyContextBase *context)
{
    if (!_newUids.isEmpty()) {
        while (!_newUids.isEmpty()) {
            QStringList uidList;
            foreach(const QString &s, _newUids.mid(0, DefaultBatchSize)) {
                uidList << ImapProtocol::uid(s);
            }

            context->protocol().sendUidFetch(MetaDataFetchFlags, IntegerRegion(uidList).toString());
            ++_outstandingPreviews;

            _newUids = _newUids.mid(uidList.count());
        }
    } else {
        // We have previewed all messages from the current folder
        folderPreviewCompleted(context);

        if (!selectNextPreviewFolder(context)) {
            // No more messages to preview
            if ((_transferState == Preview) || (_retrieveUids.isEmpty())) {
                if (!_completionList.isEmpty() || !_completionSectionList.isEmpty()) {
                    // Fetch the messages that need completion
                    clearSelection();

                    selectedMailsAppend(_completionList);

                    QList<QPair<QMailMessagePart::Location, uint> >::const_iterator it = _completionSectionList.begin(), end = _completionSectionList.end();
                    for ( ; it != end; ++it) {
                        if (it->second != 0) {
                            selectedSectionsAppend(it->first, it->second);
                        } else {
                            selectedSectionsAppend(it->first);
                        }
                    }

                    _completionList.clear();
                    _completionSectionList.clear();

                    resetMessageListTraversal();
                    messageListMessageAction(context);
                } else {
                    // No messages to be completed
                    messageListCompleted(context);
                }
            }
        }
    }
}

void ImapSynchronizeBaseStrategy::folderPreviewCompleted(ImapStrategyContextBase *)
{
}

void ImapSynchronizeBaseStrategy::recursivelyCompleteParts(ImapStrategyContextBase *context, 
                                                           const QMailMessagePartContainer &partContainer, 
                                                           int &partsToRetrieve, 
                                                           int &bytesLeft)
{
    if (partContainer.multipartType() == QMailMessage::MultipartAlternative) {
        // See if there is a preferred sub-part to retrieve
        ImapConfiguration imapCfg(context->config());

        QString preferred(imapCfg.preferredTextSubtype().toLower());
        if (!preferred.isEmpty()) {
            for (uint i = 0; i < partContainer.partCount(); ++i) {
                const QMailMessagePart part(partContainer.partAt(i));
                const QMailMessageContentDisposition disposition(part.contentDisposition());
                const QMailMessageContentType contentType(part.contentType());

                if ((contentType.type().toLower() == "text") && (contentType.subType().toLower() == preferred)) {
                    if (bytesLeft >= disposition.size()) {
                        _completionSectionList.append(qMakePair(part.location(), 0u));
                        bytesLeft -= disposition.size();
                        ++partsToRetrieve;
                    } else if (preferred == "plain") {
                        // We can retrieve the first portion of this part
                        _completionSectionList.append(qMakePair(part.location(), static_cast<unsigned>(bytesLeft)));
                        bytesLeft = 0;
                        ++partsToRetrieve;
                    }
                    return;
                }
            }
        }
    }

    // Otherwise, consider the subparts individually
    for (uint i = 0; i < partContainer.partCount(); ++i) {
        const QMailMessagePart part(partContainer.partAt(i));
        const QMailMessageContentDisposition disposition(part.contentDisposition());
        const QMailMessageContentType contentType(part.contentType());

        if (partsToRetrieve > 10) {
            break; // sanity check, prevent DOS
        } else if (part.partCount() > 0) {
            recursivelyCompleteParts(context, part, partsToRetrieve, bytesLeft);
        } else if (part.partialContentAvailable()) {
            continue;
        } else if (disposition.size() <= 0) {
            continue;
        } else if ((disposition.type() != QMailMessageContentDisposition::Inline) && (contentType.type().toLower() != "text")) {
            continue;
        } else if (bytesLeft >= disposition.size()) {
            _completionSectionList.append(qMakePair(part.location(), 0u));
            bytesLeft -= disposition.size();
            ++partsToRetrieve;
        } else if ((contentType.type().toLower() == "text") && (contentType.subType().toLower() == "plain")) {
            // We can retrieve the first portion of this part
            _completionSectionList.append(qMakePair(part.location(), static_cast<unsigned>(bytesLeft)));
            bytesLeft = 0;
            ++partsToRetrieve;
        }
    }
}

void ImapSynchronizeBaseStrategy::messageFetched(ImapStrategyContextBase *context, QMailMessage &message)
{ 
    ImapFolderListStrategy::messageFetched(context, message);

    if (_transferState == Preview) {
        context->progressChanged(_progress++, _total);

        if (message.size() < _headerLimit) {
            _completionList.append(message.id());
        } else {
            const QMailMessageContentType contentType(message.contentType());
            if ((contentType.type().toLower() == "text") && (contentType.subType().toLower() == "plain")) {
                // We can retrieve the first portion of this message
                QMailMessagePart::Location location;
                location.setContainingMessageId(message.id());
                _completionSectionList.append(qMakePair(location, static_cast<unsigned>(_headerLimit)));
            } else {
                int bytesLeft = _headerLimit;
                int partsToRetrieve = 1;
                recursivelyCompleteParts(context, message, partsToRetrieve, bytesLeft);
            }
        }
    }
}


/* A strategy to fetch the list of folders available at the server
*/
void ImapRetrieveFolderListStrategy::setBase(const QMailFolderId &folderId)
{
    _baseId = folderId;
}

void ImapRetrieveFolderListStrategy::setDescending(bool descending)
{
    _descending = descending;
}

void ImapRetrieveFolderListStrategy::newConnection(ImapStrategyContextBase *context)
{
    _mailboxList.clear();
    _ancestorPaths.clear();

    ImapSynchronizeBaseStrategy::newConnection(context);
}

void ImapRetrieveFolderListStrategy::handleLogin(ImapStrategyContextBase *context)
{
    context->updateStatus( QObject::tr("Retrieving folders") );
    _mailboxPaths.clear();

    QMailFolderId folderId;

    ImapConfiguration imapCfg(context->config());
    if (_baseId.isValid()) {
        folderId = _baseId;
    }

    _transferState = List;

    if (folderId.isValid()) {
        // Begin processing with the specified base folder
        selectedFoldersAppend(QMailFolderIdList() << folderId);
        ImapSynchronizeBaseStrategy::handleLogin(context);
    } else {
        // We need to search for folders at the account root
        context->protocol().sendList(QMailFolder(), "%");
    }
}

void ImapRetrieveFolderListStrategy::handleSearch(ImapStrategyContextBase *context)
{
    updateUndiscoveredCount(context);

    // Don't bother listing mailboxes that have no children
    FolderStatus folderState = _folderStatus[_currentMailbox.id()];
    if (!(folderState & NoInferiors) && !(folderState & HasNoChildren)) {
        // Find the child folders of this mailbox
        context->protocol().sendList(_currentMailbox, "%");
    } else {
        folderListFolderAction(context);
    }
}

void ImapRetrieveFolderListStrategy::handleList(ImapStrategyContextBase *context)
{
    if (!_currentMailbox.id().isValid()) {
        // See if there are any more ancestors to search
        if (!_ancestorSearchPaths.isEmpty()) {
            QMailFolder ancestor;
            ancestor.setPath(_ancestorSearchPaths.takeFirst());

            context->protocol().sendList(ancestor, "%");
            return;
        }
    }

    ImapSynchronizeBaseStrategy::handleList(context);
}

void ImapRetrieveFolderListStrategy::folderListCompleted(ImapStrategyContextBase *context)
{
    // We should have discovered all available mailboxes now
    _mailboxList = context->client()->mailboxIds();

    removeDeletedMailboxes(context);

    // We have retrieved all the folders - process any messages
    messageListMessageAction(context);
}

void ImapRetrieveFolderListStrategy::mailboxListed(ImapStrategyContextBase *context, QMailFolder &folder, const QString &flags)
{
    ImapSynchronizeBaseStrategy::mailboxListed(context, folder, flags);

    _mailboxPaths.append(folder.path());

    if (_descending) {
        QString path(folder.path());

        if (folder.id().isValid()) {
            if (folder.id() != _currentMailbox.id()) {
                if (_baseFolder.isEmpty() || 
                    (path.startsWith(_baseFolder, Qt::CaseInsensitive) && (path.length() == _baseFolder.length())) ||
                    (path.startsWith(_baseFolder + context->protocol().delimiter(), Qt::CaseInsensitive))) {
                    // We need to list this folder's contents, too
                    selectedFoldersAppend(QMailFolderIdList() << folder.id());
                }
            }
        } else {
            if (!_ancestorPaths.contains(path)) {
                if (_baseFolder.startsWith(path + context->protocol().delimiter(), Qt::CaseInsensitive)) {
                    // This folder must be an ancestor of the base folder - list its contents
                    _ancestorPaths.insert(path);
                    _ancestorSearchPaths.append(path);
                }
            }
        }
    }
}

void ImapRetrieveFolderListStrategy::removeDeletedMailboxes(ImapStrategyContextBase *context)
{
    // Do we have the full list of account mailboxes?
    if (_descending && !_baseId.isValid()) {
        // Find the local mailboxes that no longer exist on the server
        QMailFolderIdList nonexistent;
        foreach (const QMailFolderId &boxId, _mailboxList) {
            QMailFolder mailbox(boxId);
            bool exists = _mailboxPaths.contains(mailbox.path());
            if (!exists) {
                nonexistent.append(mailbox.id());
            }
        }

        foreach (const QMailFolderId &boxId, nonexistent) {
            // Any messages in this box should be removed also
            foreach (const QString& uid, context->client()->serverUids(boxId)) {
                // We might have a deletion record for this message
                QMailStore::instance()->purgeMessageRemovalRecords(context->config().id(), QStringList() << uid);
            }

            if (!QMailStore::instance()->removeFolder(boxId)) {
                _error = true;
                qWarning() << "Unable to remove nonexistent folder for account:" << context->config().id();
            }

            _mailboxList.removeAll(boxId);
        }
    }
}

/* A strategy to provide full account synchronization. 
   
   That is to export and import changes, to retrieve message previews for all 
   known messages in an account, and to complete messages where appropriate.
*/
ImapSynchronizeAllStrategy::ImapSynchronizeAllStrategy()
{
    _options = static_cast<Options>(RetrieveMail | ImportChanges | ExportChanges);
}

void ImapSynchronizeAllStrategy::setOptions(Options options)
{
    _options = options;
}

void ImapSynchronizeAllStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_UIDSearch:
        {
            handleUidSearch(context);
            break;
        }
        
        case IMAP_UIDStore:
        {
            handleUidStore(context);
            break;
        }

        case IMAP_Expunge:
        {
            handleExpunge(context);
            break;
        }

        default:
        {
            ImapRetrieveFolderListStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapSynchronizeAllStrategy::handleUidSearch(ImapStrategyContextBase *context)
{
    const ImapMailboxProperties &properties(context->mailbox());

    switch(_searchState)
    {
    case Seen:
    {
        _seenUids = properties.uidList;

        // The Unseen search command was pipelined
        _searchState = Unseen;
        break;
    }
    case Unseen:
    {
        _unseenUids = properties.uidList;

        // The Flagged search command was pipelined
        _searchState = Flagged;
        break;
    }
    case Flagged:
    {
        _flaggedUids = properties.uidList;
        if (static_cast<quint32>((_unseenUids.count() + _seenUids.count())) == properties.exists) {
            // We have a consistent set of search results
            processUidSearchResults(context);
        } else {
            qMailLog(IMAP) << "Inconsistent UID SEARCH result using SEEN/UNSEEN; reverting to ALL";

            // Try doing a search for ALL messages
            _unseenUids.clear();
            _seenUids.clear();
            _flaggedUids.clear();
            _searchState = All;
            context->protocol().sendUidSearch(MFlag_All);
        }
        break;
    }
    case All:
    {
        _unseenUids = properties.uidList;
        if (static_cast<quint32>(_unseenUids.count()) != properties.exists) {
            qMailLog(IMAP) << "Inconsistent UID SEARCH result";

            // No consistent search result, so don't delete anything
            _searchState = Inconclusive;
        }

        processUidSearchResults(context);
        break;
    }
    default:
        qMailLog(IMAP) << "Unknown search status in transition";
    }
}

void ImapSynchronizeAllStrategy::handleUidStore(ImapStrategyContextBase *context)
{
    if (!(_options & ExportChanges)) {
        processNextFolder(context);
        return;
    }

    if (!_storedReadUids.isEmpty()) {
        // Mark messages as read locally that have been marked as read on the server
        QMailMessageKey readKey(context->client()->messagesKey(_currentMailbox.id()) & QMailMessageKey::serverUid(_storedReadUids));
        if (QMailStore::instance()->updateMessagesMetaData(readKey, QMailMessage::ReadElsewhere, true)) {
            _storedReadUids.clear();
        } else {
            _error = true;
            qWarning() << "Unable to update marked as read message metadata for account:" << context->config().id() << "folder" << _currentMailbox.id();
        }
    }
    if (!_storedUnreadUids.isEmpty()) {
        // Mark messages as unread locally that have been marked as unread on the server
        QMailMessageKey unreadKey(context->client()->messagesKey(_currentMailbox.id()) & QMailMessageKey::serverUid(_storedUnreadUids));
        if (QMailStore::instance()->updateMessagesMetaData(unreadKey, QMailMessage::ReadElsewhere, false)) {
            _storedUnreadUids.clear();
        } else {
            _error = true;
            qWarning() << "Unable to update marked as unread message metadata for account:" << context->config().id() << "folder" << _currentMailbox.id();
        }
    }
    if (!_storedImportantUids.isEmpty()) {
        // Mark messages as important locally that have been marked as important on the server
        QMailMessageKey importantKey(context->client()->messagesKey(_currentMailbox.id()) & QMailMessageKey::serverUid(_storedImportantUids));
        if (QMailStore::instance()->updateMessagesMetaData(importantKey, QMailMessage::ImportantElsewhere, true)) {
            _storedImportantUids.clear();
        } else {
            _error = true;
            qWarning() << "Unable to update marked as important message metadata for account:" << context->config().id() << "folder" << _currentMailbox.id();
        }
    }
    if (!_storedUnimportantUids.isEmpty()) {
        // Mark messages as unimportant locally that have been marked as unimportant on the server
        QMailMessageKey unimportantKey(context->client()->messagesKey(_currentMailbox.id()) & QMailMessageKey::serverUid(_storedUnimportantUids));
        if (QMailStore::instance()->updateMessagesMetaData(unimportantKey, QMailMessage::ImportantElsewhere, false)) {
            _storedUnimportantUids.clear();
        } else {
            _error = true;
            qWarning() << "Unable to update marked as unimportant message metadata for account:" << context->config().id() << "folder" << _currentMailbox.id();
        }
    }
    
    if (!setNextSeen(context) && !setNextNotSeen(context) && !setNextImportant(context) && !setNextNotImportant(context) && !setNextDeleted(context)) {
        if (!_storedRemovedUids.isEmpty()) {
            // Remove records of deleted messages, after EXPUNGE
            if (QMailStore::instance()->purgeMessageRemovalRecords(context->config().id(), _storedRemovedUids)) {
                _storedRemovedUids.clear();
            } else {
                _error = true;
                qWarning() << "Unable to purge message record for account:" << context->config().id() << "folder" << _currentMailbox.id();
            }
        }
        
        processNextFolder(context);
    }
}

void ImapSynchronizeAllStrategy::handleExpunge(ImapStrategyContextBase *context)
{
    processNextFolder(context);
}

void ImapSynchronizeAllStrategy::folderListFolderAction(ImapStrategyContextBase *context)
{
    _seenUids = QStringList();
    _unseenUids = QStringList();
    _flaggedUids = QStringList();
    _readUids = QStringList();
    _unreadUids = QStringList();
    _importantUids = QStringList();
    _unimportantUids = QStringList();

    _removedUids = QStringList();
    _expungeRequired = false;
    
    // Search for messages in the current mailbox
    _searchState = Seen;

    if (context->mailbox().exists > 0) {
        // Start by looking for previously-seen and unseen messages
        context->protocol().sendUidSearch(MFlag_Seen);
        context->protocol().sendUidSearch(MFlag_Unseen);
        context->protocol().sendUidSearch(MFlag_Flagged);
    } else {
        // No messages, so no need to perform search
        processUidSearchResults(context);
    }
}

void ImapSynchronizeAllStrategy::processUidSearchResults(ImapStrategyContextBase *context)
{
    QMailFolderId boxId = _currentMailbox.id();
    QMailMessageKey accountKey(QMailMessageKey::parentAccountId(context->config().id()));
    QMailMessageKey partialContentKey(QMailMessageKey::status(QMailMessage::PartialContentAvailable));
    QMailFolder folder(boxId);
    
    if ((_currentMailbox.status() & QMailFolder::SynchronizationEnabled) &&
        !(_currentMailbox.status() & QMailFolder::Synchronized)) {
        // We have just synchronized this folder
        folder.setStatus(QMailFolder::Synchronized, true);
    }
    if (!QMailStore::instance()->updateFolder(&folder)) {
        _error = true;
        qWarning() << "Unable to update folder for account:" << context->config().id();
    }

    // Messages reported as being on the server
    QStringList reportedOnServerUids = _seenUids + _unseenUids;

    // Messages (with at least metadata) stored locally
    QMailMessageKey folderKey(context->client()->messagesKey(boxId) | context->client()->trashKey(boxId));
    QStringList deletedUids = context->client()->deletedMessages(boxId);
    QMailMessageKey storedKey((accountKey & folderKey & partialContentKey) | QMailMessageKey::serverUid(deletedUids) );

    // New messages reported by the server that we don't yet have
    if (_options & RetrieveMail) {
        // Opportunity for optimization here

        QStringList newUids(inFirstButNotSecond(reportedOnServerUids, context->client()->serverUids(storedKey)));
        if (!newUids.isEmpty()) {
            // Add this folder to the list to retrieve from later
            _retrieveUids.append(qMakePair(boxId, newUids));
        }
    }

    if (_searchState == Inconclusive) {
        // Don't mark or delete any messages without a correct server listing
        searchInconclusive(context);
    } else {
        QMailMessageKey readStatusKey(QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Includes));
        QMailMessageKey removedStatusKey(QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Includes));
        QMailMessageKey unreadElsewhereKey(folderKey & accountKey & ~readStatusKey);
        QMailMessageKey unavailableKey(folderKey & accountKey & removedStatusKey);
        QMailMessageKey unseenKey(QMailMessageKey::serverUid(_unseenUids));
        QMailMessageKey seenKey(QMailMessageKey::serverUid(_seenUids));
        QMailMessageKey flaggedKey(QMailMessageKey::serverUid(_flaggedUids));
        QMailMessageKey importantElsewhereKey(QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Includes));
        QMailMessageKey movedKey(QMailMessageKey::previousParentFolderId(QMailFolderKey::parentAccountId(context->config().id()),QMailDataComparator::Includes));
        
        // Only delete messages the server still has
        _removedUids = inFirstAndSecond(deletedUids, reportedOnServerUids);
        _expungeRequired = !_removedUids.isEmpty();

        if (_options & ImportChanges) {
            if (!updateMessagesMetaData(context, storedKey, unseenKey, seenKey, flaggedKey, unreadElsewhereKey, importantElsewhereKey, unavailableKey, movedKey)) {
                _error = true;
            }
        }
        
        // Update messages on the server that are still flagged as unseen but have been read locally
        QMailMessageKey readHereKey(folderKey 
                                    & accountKey
                                    & QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Includes)
                                    & QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Excludes)
                                    & QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes)
                                    & unseenKey);
        _readUids = context->client()->serverUids(readHereKey);

        // Update messages on the server that are flagged as seen but have been explicitly marked as unread locally
        QMailMessageKey markedAsUnreadHereKey(folderKey 
                                    & accountKey
                                    & QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Excludes)
                                    & QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Includes)
                                    & QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes)
                                    & seenKey);
        _unreadUids = context->client()->serverUids(markedAsUnreadHereKey);

        // Update messages on the server that are still not flagged as important but have been flagged as important locally
        QMailMessageKey reportedKey(seenKey | unseenKey);
        QMailMessageKey unflaggedKey(reportedKey & ~flaggedKey);
        QMailMessageKey markedAsImportantHereKey(folderKey 
                                                 & accountKey
                                                 & QMailMessageKey::status(QMailMessage::Important, QMailDataComparator::Includes)
                                                 & QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Excludes)
                                                 & QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes)
                                                 & unflaggedKey);
        _importantUids = context->client()->serverUids(markedAsImportantHereKey);

        // Update messages on the server that are still flagged as important but have been flagged as not important locally
        QMailMessageKey markedAsNotImportantHereKey(folderKey 
                                                    & accountKey
                                                    & QMailMessageKey::status(QMailMessage::Important, QMailDataComparator::Excludes)
                                                    & QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Includes)
                                                    & QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes)
                                                    & flaggedKey);
        _unimportantUids = context->client()->serverUids(markedAsNotImportantHereKey);

        // Mark any messages that we have read that the server thinks are unread
        handleUidStore(context);
    }
}

void ImapSynchronizeAllStrategy::searchInconclusive(ImapStrategyContextBase *context)
{
    processNextFolder(context);
}

bool ImapSynchronizeAllStrategy::setNextSeen(ImapStrategyContextBase *context)
{
    if (!_readUids.isEmpty()) {
        QStringList msgUidl = _readUids.mid(0, batchSize);
        QString msg = QObject::tr("Marking message %1 read").arg(msgUidl.first());
        foreach(QString uid, msgUidl) {
            _readUids.removeAll(uid);
            _storedReadUids.append(uid);
        }

        context->updateStatus(msg);
        context->protocol().sendUidStore(MFlag_Seen, true, numericUidSequence(msgUidl));
        
        return true;
    }

    return false;
}

bool ImapSynchronizeAllStrategy::setNextNotSeen(ImapStrategyContextBase *context)
{
    if (!_unreadUids.isEmpty()) {
        QStringList msgUidl = _unreadUids.mid(0, batchSize);
        QString msg = QObject::tr("Marking message %1 unread").arg(msgUidl.first());
        foreach(QString uid, msgUidl) {
            _unreadUids.removeAll(uid);
            _storedUnreadUids.append(uid);
        }

        context->updateStatus(msg);
        context->protocol().sendUidStore(MFlag_Seen, false, numericUidSequence(msgUidl));
        
        return true;
    }

    return false;
}

bool ImapSynchronizeAllStrategy::setNextImportant(ImapStrategyContextBase *context)
{
    if (!_importantUids.isEmpty()) {
        QStringList msgUidl = _importantUids.mid(0, batchSize);
        QString msg = QObject::tr("Marking message %1 important").arg(msgUidl.first());
        foreach(QString uid, msgUidl) {
            _importantUids.removeAll(uid);
            _storedImportantUids.append(uid);
        }

        context->updateStatus(msg);
        context->protocol().sendUidStore(MFlag_Flagged, true, numericUidSequence(msgUidl));
        
        return true;
    }

    return false;
}

bool ImapSynchronizeAllStrategy::setNextNotImportant(ImapStrategyContextBase *context)
{
    if (!_unimportantUids.isEmpty()) {
        QStringList msgUidl = _unimportantUids.mid(0, batchSize);
        QString msg = QObject::tr("Marking message %1 unimportant").arg(msgUidl.first());
        foreach(QString uid, msgUidl) {
            _unimportantUids.removeAll(uid);
            _storedUnimportantUids.append(uid);
        }

        context->updateStatus(msg);
        context->protocol().sendUidStore(MFlag_Flagged, false, numericUidSequence(msgUidl));
        
        return true;
    }

    return false;
}

bool ImapSynchronizeAllStrategy::setNextDeleted(ImapStrategyContextBase *context)
{
    ImapConfiguration imapCfg(context->config());
    if (imapCfg.canDeleteMail()) {
        if (!_removedUids.isEmpty()) {
            QStringList msgUidl = _removedUids.mid(0, batchSize);
            QString msg = QObject::tr("Deleting message %1").arg(msgUidl.first());
            foreach(QString uid, msgUidl) {
                _removedUids.removeAll(uid);
                _storedRemovedUids.append(uid);
            }

            context->updateStatus(msg);
            context->protocol().sendUidStore(MFlag_Deleted, true, numericUidSequence(msgUidl));
        } else if (_expungeRequired) {
            // All messages flagged as deleted, expunge them
            context->protocol().sendExpunge();
            return true;
        }
    }

    return false;
}

void ImapSynchronizeAllStrategy::folderPreviewCompleted(ImapStrategyContextBase *context)
{
    const ImapMailboxProperties &properties(context->mailbox());

    if (!_error && (properties.exists > 0)) {
        QMailFolder folder(properties.id);
        folder.setCustomField("qmf-min-serveruid", QString::number(1));
        folder.setCustomField("qmf-max-serveruid", QString::number(properties.uidNext - 1));
        folder.setServerUndiscoveredCount(0);

        if (!QMailStore::instance()->updateFolder(&folder)) {
            _error = true;
            qWarning() << "Unable to update folder for account:" << context->config().id();
        }
    }
}


/* A strategy to retrieve all messages from an account.
   
   That is to retrieve message previews for all known messages 
   in an account, and to complete messages where appropriate.
*/
ImapRetrieveAllStrategy::ImapRetrieveAllStrategy()
{
    // This is just synchronize without update-exporting
    setOptions(static_cast<Options>(RetrieveMail | ImportChanges));
}


/* A strategy to exports changes made on the client to the server.
   That is to export status flag changes (unread <-> read) and deletes.
*/
ImapExportUpdatesStrategy::ImapExportUpdatesStrategy()
{
    setOptions(ExportChanges);
}

void ImapExportUpdatesStrategy::handleLogin(ImapStrategyContextBase *context)
{
    _transferState = List;
    _completionList.clear();
    _completionSectionList.clear();

    ImapConfiguration imapCfg(context->config());
    if (!imapCfg.canDeleteMail()) {
        QString name(QMailAccount(context->config().id()).name());
        qMailLog(Messaging) << "Not exporting deletions. Deleting mail is disabled for account name" << name;
    }

    QMailMessageKey readStatusKey(QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Includes));
    readStatusKey &= QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Excludes);
    readStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);
    QMailMessageKey unreadStatusKey(QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Excludes));
    unreadStatusKey &= QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Includes);
    unreadStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);
    QMailMessageKey importantStatusKey(QMailMessageKey::status(QMailMessage::Important, QMailDataComparator::Includes));
    importantStatusKey &= QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Excludes);
    importantStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);
    QMailMessageKey unimportantStatusKey(QMailMessageKey::status(QMailMessage::Important, QMailDataComparator::Excludes));
    unimportantStatusKey &= QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Includes);
    unimportantStatusKey &= QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Excludes);

    _folderMessageUids.clear();

    ImapClient *c(context->client());
    foreach (const QMailFolderId &folderId, context->client()->mailboxIds()) {
        QMailMessageKey folderKey(c->messagesKey(folderId) | c->trashKey(folderId));

        // Find messages marked as read locally
        QStringList deletedUids;
        QStringList readUids = c->serverUids(folderKey & readStatusKey);
        QStringList unreadUids = c->serverUids(folderKey & unreadStatusKey);
        QStringList importantUids = c->serverUids(folderKey & importantStatusKey);
        QStringList unimportantUids = c->serverUids(folderKey & unimportantStatusKey);        

        if (imapCfg.canDeleteMail()) {
            // Also find messages deleted locally
            deletedUids = context->client()->deletedMessages(folderId);
        }

        if (!readUids.isEmpty() || !unreadUids.isEmpty() || !importantUids.isEmpty() || !unimportantUids.isEmpty() || !deletedUids.isEmpty()) {
            QList<QStringList> entry;
            entry << readUids << unreadUids << importantUids << unimportantUids << deletedUids;
            _folderMessageUids.insert(folderId, entry);
        }
    }
    
    processNextFolder(context);
}

void ImapExportUpdatesStrategy::handleUidSearch(ImapStrategyContextBase *context)
{
    _serverReportedUids = context->mailbox().uidList;

    processUidSearchResults(context);
}

void ImapExportUpdatesStrategy::folderListFolderAction(ImapStrategyContextBase *context)
{
    _serverReportedUids = QStringList();
    
    // We have selected the current mailbox
    if (context->mailbox().exists > 0) {
        // Find which of our messages-of-interest are still on the server
        IntegerRegion clientRegion(stripFolderPrefix(_clientReadUids + _clientUnreadUids + _clientImportantUids + _clientUnimportantUids + _clientDeletedUids));
        context->protocol().sendUidSearch(MFlag_All, "UID " + clientRegion.toString());
    } else {
        // No messages, so no need to perform search
        processUidSearchResults(context);
    }
}

bool ImapExportUpdatesStrategy::nextFolder()
{
    if (_folderMessageUids.isEmpty()) {
        return false;
    }

    QMap<QMailFolderId, QList<QStringList> >::iterator it = _folderMessageUids.begin();

    if (it.value().count() != 5) {
        qWarning() << "quintuple mismatch in export updates nextFolder, folder" << it.key() << "count" << it.value().count();
        it = _folderMessageUids.erase(it);
        return nextFolder();
    }
    
    setCurrentMailbox(it.key());
    _clientReadUids = it.value()[0];
    _clientUnreadUids = it.value()[1];
    _clientImportantUids = it.value()[2];
    _clientUnimportantUids = it.value()[3];
    _clientDeletedUids = it.value()[4];

    it = _folderMessageUids.erase(it);
    return true;
}

void ImapExportUpdatesStrategy::processUidSearchResults(ImapStrategyContextBase *context)
{
    // Messages marked deleted locally that the server reports exists
    _removedUids = inFirstAndSecond(_clientDeletedUids, _serverReportedUids);
    _expungeRequired = !_removedUids.isEmpty();

    // Messages marked read locally that the server reports exists
    _readUids = inFirstAndSecond(_clientReadUids, _serverReportedUids);

    // Messages marked unread locally that the server reports exists
    _unreadUids = inFirstAndSecond(_clientUnreadUids, _serverReportedUids);

    // Messages marked important locally that the server reports are unimportant
    _importantUids = inFirstAndSecond(_clientImportantUids, _serverReportedUids);

    // Messages marked unimportant locally that the server reports are important
    _unimportantUids = inFirstAndSecond(_clientUnimportantUids, _serverReportedUids);

    handleUidStore(context);
}


/* A strategy to update message flags for a list of messages.
   
   That is to detect changes to flags (unseen->seen) 
   and to detect deleted mails.
*/
void ImapUpdateMessagesFlagsStrategy::clearSelection()
{
    ImapFolderListStrategy::clearSelection();

    _selectedMessageIds.clear();
    _folderMessageUids.clear();
}

void ImapUpdateMessagesFlagsStrategy::selectedMailsAppend(const QMailMessageIdList &messageIds)
{
    _selectedMessageIds += messageIds;
}

void ImapUpdateMessagesFlagsStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_UIDSearch:
        {
            handleUidSearch(context);
            break;
        }
        
        default:
        {
            ImapFolderListStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapUpdateMessagesFlagsStrategy::handleLogin(ImapStrategyContextBase *context)
{
    _transferState = List;
    _serverUids.clear();
    _searchState = Unseen;

    // Associate each message to the relevant folder
    _folderMessageUids.clear();
    if (!_selectedMessageIds.isEmpty()) {
        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(_selectedMessageIds), 
                                                                                                QMailMessageKey::ServerUid | QMailMessageKey::ParentFolderId, 
                                                                                                QMailStore::ReturnAll)) {
            if (!metaData.serverUid().isEmpty() && metaData.parentFolderId().isValid())
                _folderMessageUids[metaData.parentFolderId()].append(metaData.serverUid());
        }
    }

    processNextFolder(context);
}

void ImapUpdateMessagesFlagsStrategy::handleUidSearch(ImapStrategyContextBase *context)
{
    const ImapMailboxProperties &properties(context->mailbox());

    switch(_searchState)
    {
    case Unseen:
    {
        _unseenUids = properties.uidList;

        // The Seen search command was pipelined
        _searchState = Seen;
        break;
    }
    case Seen:
    {
        _seenUids = properties.uidList;

        // The Flagged search command was pipelined
        _searchState = Flagged;
        break;
    }
    case Flagged:
    {
        _flaggedUids = properties.uidList;
        processUidSearchResults(context);
        break;
    }
    default:
        qMailLog(IMAP) << "Unknown search status in transition";
        Q_ASSERT(0);

        processNextFolder(context);
    }
}

void ImapUpdateMessagesFlagsStrategy::folderListFolderAction(ImapStrategyContextBase *context)
{
    const ImapMailboxProperties &properties(context->mailbox());

    if (!properties.noModSeq && (properties.highestModSeq == _currentModSeq)) {
        // The mod sequence has not changed for this folder
        processNextFolder(context);
        return;
    }

    //  If we have messages, we need to discover any flag changes
    if (properties.exists > 0) {
        IntegerRegion clientRegion(stripFolderPrefix(_serverUids));
        _filter = clientRegion.toString();
        _searchState = Unseen;

        // Start by looking for previously-unseen messages
        // If a message is moved from Unseen to Seen between our two searches, then we will
        // miss this change by searching Unseen first; if, however, we were to search Seen first,
        // then we would miss the message altogether and mark it as deleted...
        context->protocol().sendUidSearch(MFlag_Unseen, "UID " + _filter);
        context->protocol().sendUidSearch(MFlag_Seen, "UID " + _filter);
        context->protocol().sendUidSearch(MFlag_Flagged, "UID " + _filter);
    } else {
        processUidSearchResults(context);
    }
}

void ImapUpdateMessagesFlagsStrategy::folderListCompleted(ImapStrategyContextBase *context)
{
    messageListCompleted(context);
}

bool ImapUpdateMessagesFlagsStrategy::nextFolder()
{
    if (_folderMessageUids.isEmpty()) {
        return false;
    }

    QMap<QMailFolderId, QStringList>::iterator it = _folderMessageUids.begin();

    setCurrentMailbox(it.key());

    _serverUids = it.value();

    it = _folderMessageUids.erase(it);
    return true;
}

void ImapUpdateMessagesFlagsStrategy::processFolder(ImapStrategyContextBase *context)
{
    QMailFolderId folderId(_currentMailbox.id());
    
    //not not try select an unselectable mailbox
    if(!_folderStatus.contains(folderId) || !(_folderStatus.value(folderId) & NoSelect))
        context->protocol().sendSelect(_currentMailbox);
}

void ImapUpdateMessagesFlagsStrategy::processUidSearchResults(ImapStrategyContextBase *context)
{
    QMailFolderId folderId(_currentMailbox.id());
    if (!folderId.isValid()) {
        // Folder was removed while we were updating messages flags in it
        processNextFolder(context);
        return;
    }
    
    // Compare the server message list with our message list
    QMailMessageKey accountKey(QMailMessageKey::parentAccountId(context->config().id()));
    QMailMessageKey storedKey(QMailMessageKey::serverUid(_serverUids));
    QMailMessageKey unseenKey(QMailMessageKey::serverUid(_unseenUids));
    QMailMessageKey seenKey(QMailMessageKey::serverUid(_seenUids));
    QMailMessageKey readStatusKey(QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Includes));
    QMailMessageKey removedStatusKey(QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Includes));
    QMailMessageKey folderKey(context->client()->messagesKey(folderId) | context->client()->trashKey(folderId));
    QMailMessageKey unreadElsewhereKey(folderKey & accountKey & ~readStatusKey);
    QMailMessageKey unavailableKey(folderKey & accountKey & removedStatusKey);
    QMailMessageKey flaggedKey(QMailMessageKey::serverUid(_flaggedUids));
    QMailMessageKey importantElsewhereKey(QMailMessageKey::status(QMailMessage::ImportantElsewhere, QMailDataComparator::Includes));
    QMailMessageKey movedKey(QMailMessageKey::previousParentFolderId(QMailFolderKey::parentAccountId(context->config().id()),QMailDataComparator::Includes));

    if (!updateMessagesMetaData(context, storedKey, unseenKey, seenKey, flaggedKey, unreadElsewhereKey, importantElsewhereKey, unavailableKey, movedKey))
        _error = true;

    processNextFolder(context);
}


/* A strategy to ensure a given number of messages, for a given
   mailbox are on the client, retrieving messages if necessary.

   Retrieval order is defined by server uid.
*/
void ImapRetrieveMessageListStrategy::setMinimum(uint minimum)
{
    _minimum = minimum;
    _mailboxIds.clear();
}

void ImapRetrieveMessageListStrategy::setAccountCheck(bool check)
{
    _accountCheck = check;
}

void ImapRetrieveMessageListStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_UIDSearch:
        {
            handleUidSearch(context);
            break;
        }
        
        default:
        {
            ImapSynchronizeBaseStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapRetrieveMessageListStrategy::handleLogin(ImapStrategyContextBase *context)
{
    context->updateStatus(QObject::tr("Scanning folder"));
    _transferState = List;
    _fillingGap = false;
    _completionList.clear();
    _completionSectionList.clear();
    _newMinMaxMap.clear();
    _listAll = false;
    
    ImapSynchronizeBaseStrategy::handleLogin(context);
}

void ImapRetrieveMessageListStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    foreach (const QMailFolderId &folderId, _updatedFolders) {
        QMailFolder folder(folderId);

        bool modified(false);
        if (!_error // Only update the range of downloaded messages if no store errors, including add errors have occurred
            && _newMinMaxMap.contains(folderId)) {
            folder.setCustomField("qmf-min-serveruid", QString::number(_newMinMaxMap[folderId].minimum()));
            folder.setCustomField("qmf-max-serveruid", QString::number(_newMinMaxMap[folderId].maximum()));
            modified = true;
        }

        if (folder.serverUndiscoveredCount() != 0) {
            folder.setServerUndiscoveredCount(0); // All undiscovered messages have been retrieved
            modified = true;
        }

        if (modified && !QMailStore::instance()->updateFolder(&folder)) {
            _error = true;
            qWarning() << "Unable to update folder for account:" << context->config().id();
        }
    }

    _updatedFolders.clear();
    _newMinMaxMap.clear();
    ImapSynchronizeBaseStrategy::messageListCompleted(context);
}

void ImapRetrieveMessageListStrategy::folderListCompleted(ImapStrategyContextBase *context)
{
    previewDiscoveredMessages(context);
}

void ImapRetrieveMessageListStrategy::handleUidSearch(ImapStrategyContextBase *context)
{
    const ImapMailboxProperties &properties(context->mailbox());

    QMailFolder folder(properties.id);

    bool ok; // toUint returns 0 on error, which is an invalid IMAP uid
    int clientMin(folder.customField("qmf-min-serveruid").toUInt(&ok));
    int clientMax(folder.customField("qmf-max-serveruid").toUInt(&ok));

    // 
    // Determine which messages are on the client but not the server
    // (i.e. determine which messages have been deleted by other clients,
    //       or have been marked as deleted on the client but have not yet 
    //       been removed from the server using exportUpdates)
    //
    // The set of messages to fetch needs to be adjusted based on the number of such messages
    IntegerRegion rawServerRegion;
    foreach(const QString &uid, properties.uidList) {
        bool ok;
        uint number = stripFolderPrefix(uid).toUInt(&ok);
        if (ok)
            rawServerRegion.add(number);
    }

    int serverMinimum = properties.uidNext;
    int serverMaximum = properties.uidNext;
    if (rawServerRegion.cardinality()) {
        // Found region on server
        serverMinimum = rawServerRegion.minimum();
        if (_listAll) {
            // Considering all messages on the client in the folder
            IntegerRegion beginningClientRegion; // none of these messages are on the server
            foreach (const QMailMessageMetaData& r, QMailStore::instance()->messagesMetaData(QMailMessageKey::parentFolderId(folder.id()), QMailMessageKey::ServerUid)) {
                const QString uid(r.serverUid());
                int serverUid(stripFolderPrefix(uid).toUInt());
                if (serverUid < serverMinimum)
                    beginningClientRegion.add(serverUid);
            }
            
            QStringList removedList;
            foreach(QString uid, beginningClientRegion.toStringList()) {
                removedList.append(QString::number(folder.id().toULongLong()) + UID_SEPARATOR + uid);
            }
            QMailMessageKey removedKey(QMailMessageKey::serverUid(removedList));
            if (!QMailStore::instance()->updateMessagesMetaData(removedKey, QMailMessage::Removed, true)) {
                qWarning() << "Unable to update removed message metadata for folder:" << folder.displayName();
            }
        }
    }
    
    IntegerRegion serverRange(serverMinimum, serverMaximum);
    IntegerRegion serverComplement(serverRange.subtract(rawServerRegion));
    QStringList removedOnServerUids;
    foreach(QString uid, serverComplement.toStringList()) {
        removedOnServerUids.append(QString::number(folder.id().toULongLong()) + UID_SEPARATOR + uid);
    }

    QMailMessageKey onClientButNotServerKey(QMailMessageKey::serverUid(removedOnServerUids));
    onClientButNotServerKey &= ~QMailMessageKey::status(QMailMessage::Temporary);
    onClientButNotServerKey &= QMailMessageKey::parentFolderId(folder.id());
    QStringList removed;
    foreach (const QMailMessageMetaData& r, QMailStore::instance()->messagesMetaData(onClientButNotServerKey, QMailMessageKey::ServerUid)) {
        const QString uid(r.serverUid());
        int serverUid(stripFolderPrefix(uid).toUInt());
        if ((serverUid >= serverMinimum) && (serverUid <= serverMaximum)) {
            removed += uid;
        }
    }
    
    // The list of existing messages on the server can be used to update the status of
    // messages on the client i.e. mark messages on the client as removed for messages that have
    // been removed on the server
    QMailMessageKey removedKey(QMailMessageKey::serverUid(removed));
    if (!QMailStore::instance()->updateMessagesMetaData(removedKey, QMailMessage::Removed, true)) {
        qWarning() << "Unable to update removed message metadata for folder:" << folder.displayName();
    }
    
    // Use an optimization/simplification because client region should be contiguous
    IntegerRegion clientRegion;
    if ((clientMin != 0) && (clientMax != 0))
        clientRegion = IntegerRegion(clientMin, clientMax);
    
    IntegerRegion serverRegion(rawServerRegion);
    if (!_fillingGap) {
        if (!clientRegion.isEmpty() && !serverRegion.isEmpty()) {
            uint newestClient(clientRegion.maximum());
            uint oldestServer(serverRegion.minimum());
            uint newestServer(serverRegion.maximum());
            if (newestClient + 1 < oldestServer) {
                // There's a gap
                _fillingGap = true;
                _listAll = true;
                context->protocol().sendUidSearch(MFlag_All, QString("UID %1:%2").arg(clientRegion.minimum()).arg(newestServer));
                return;
            }
        }
    }
    
    IntegerRegion serverNew(serverRegion.subtract(IntegerRegion(1, clientMax)));
    IntegerRegion serverOld(serverRegion.subtract(IntegerRegion(clientMax + 1, INT_MAX)));
    // Adjust for messages on client not on server
    int adjustment(removed.count());
    if (_listAll) {
        // It's possible the server region extends to the beginning of the folder
        // In this case insufficient information is available to adjust accurately
        adjustment = 0;
    }
    // The idea here is that if the client has say n messages in a folder and 
    // minimum is set to n then don't get any more messages even if some messages 
    // are marked as removed on the client.
    serverOld = IntegerRegion(serverOld.toStringList().mid(adjustment));
    // a + b = c - (c - a - b)
    if (!serverRegion.isEmpty()) {
        IntegerRegion c(serverRegion.minimum(), serverRegion.maximum());
        serverRegion = c.subtract(c.subtract(serverNew).subtract(serverOld));
    }
    _updatedFolders.append(properties.id);
    _listAll = false;
    
    IntegerRegion difference(serverRegion.subtract(clientRegion));
    if (difference.cardinality()) {
        _retrieveUids.append(qMakePair(properties.id, difference.toStringList()));
        int newClientMin;
        int newClientMax;
        if (clientMin > 0)
            newClientMin = qMin(serverRegion.minimum(), clientMin);
        else
            newClientMin = serverRegion.minimum();
        if (clientMax > 0)
            newClientMax = qMax(serverRegion.maximum(), clientMax);
        else
            newClientMax = serverRegion.maximum();
        if ((newClientMin > 0) && (newClientMax > 0))
            _newMinMaxMap.insert(properties.id, IntegerRegion(newClientMin, newClientMax));
    }

    processUidSearchResults(context);
}

void ImapRetrieveMessageListStrategy::folderListFolderAction(ImapStrategyContextBase *context)
{
    // The current mailbox is now selected
    const ImapMailboxProperties &properties(context->mailbox());
    uint minimum(_minimum);
    if (_accountCheck) {
        // Request all (non search) messages in this folder or _minimum which ever is greater
        QMailMessageKey countKey(QMailMessageKey::parentFolderId(properties.id));
        countKey &= ~QMailMessageKey::status(QMailMessage::Temporary);
        minimum = qMax((uint)QMailStore::instance()->countMessages(countKey), _minimum);
    }   
    _fillingGap = false;
    _listAll = false;

    // Could get flag changes mod sequences when CONDSTORE is available
    if ((properties.exists == 0) || (minimum <= 0)) {
        // No messages, so no need to perform search
        if (properties.exists == 0) {
            // Folder is completely empty mark all messages in it on client as removed
            QMailMessageKey removedKey(QMailMessageKey::parentFolderId(properties.id));
            if (!QMailStore::instance()->updateMessagesMetaData(removedKey, QMailMessage::Removed, true)) {
                qWarning() << "Unable to update removed messag data in empty folder:" << QMailFolder(properties.id).displayName();
            }
            
        }
        processUidSearchResults(context);
        return;
    }

    // Compute starting sequence number
    int start = static_cast<int>(properties.exists) - minimum + 1;
    // In the case of retrieving new messages only (minimum = 1) and multiple messages 
    // have arrived could possibly save a round trip by: 
    //   start -= QMailFolder(properties.id).serverUndiscoveredCount(); 
    if (start < 1)
        start = 1;

    if (start == 1)
        _listAll = true;
    // We need to determine these values again
    context->protocol().sendUidSearch(MFlag_All, QString("%1:*").arg(start));
}

void ImapRetrieveMessageListStrategy::processUidSearchResults(ImapStrategyContextBase *context)
{
    processNextFolder(context);
}


/* A strategy to copy selected messages.
*/
// Note: the copy strategy is:
// NB: UIDNEXT doesn't work as expected with Cyrus 2.2; instead, just use the \Recent flag to identify copies...
//
// 1. Select the destination folder, resetting the Recent flag
// 2. Copy each specified message from its existing folder to the destination
// 3. Select the destination folder
// 4. Search for recent messages (unless we already have them because of UIDPLUS)
// 5. Retrieve metadata only for found messages
// 6. Delete any obsolete messages that were replaced by updated copies

void ImapCopyMessagesStrategy::appendMessageSet(const QMailMessageIdList &messageIds, const QMailFolderId& folderId)
{
    _messageSets.append(qMakePair(messageIds, folderId));
}

void ImapCopyMessagesStrategy::clearSelection()
{
    ImapFetchSelectedMessagesStrategy::clearSelection();
    _messageSets.clear();
}

void ImapCopyMessagesStrategy::newConnection(ImapStrategyContextBase *context)
{
    _sourceUid.clear();
    _sourceUids.clear();
    _sourceIndex = 0;
    _obsoleteDestinationUids.clear();

    ImapFetchSelectedMessagesStrategy::newConnection(context);
}

void ImapCopyMessagesStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_UIDCopy:
        {
            handleUidCopy(context);
            break;
        }
        
        case IMAP_Append:
        {
            handleAppend(context);
            break;
        }
        
        case IMAP_UIDSearch:
        {
            handleUidSearch(context);
            break;
        }
        
        case IMAP_UIDStore:
        {
            handleUidStore(context);
            break;
        }
        
        default:
        {
            ImapFetchSelectedMessagesStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapCopyMessagesStrategy::handleLogin(ImapStrategyContextBase *context)
{
    selectMessageSet(context);
}

void ImapCopyMessagesStrategy::handleSelect(ImapStrategyContextBase *context)
{
    if (_transferState == Init) {
        // Begin copying messages
        messageListMessageAction(context);
    } else if (_transferState == Search) {
        if (_createdUids.isEmpty()) {
            // We have selected the destination folder - find the newly added messages
            context->protocol().sendUidSearch(MFlag_Recent);
        } else {
            // We already have the UIDs via UIDPLUS
            removeObsoleteUids(context);
        }
    } else {
        ImapFetchSelectedMessagesStrategy::handleSelect(context);
    }
}

void ImapCopyMessagesStrategy::handleUidCopy(ImapStrategyContextBase *context)
{
    messageListMessageAction(context);
}

void ImapCopyMessagesStrategy::handleAppend(ImapStrategyContextBase *context)
{
    messageListMessageAction(context);
}

void ImapCopyMessagesStrategy::handleUidSearch(ImapStrategyContextBase *context)
{
    _createdUids = context->mailbox().uidList;

    removeObsoleteUids(context);
}

void ImapCopyMessagesStrategy::handleUidStore(ImapStrategyContextBase *context)
{
    fetchNextCopy(context);
}

void ImapCopyMessagesStrategy::handleUidFetch(ImapStrategyContextBase *context)
{
    fetchNextCopy(context);
}

void ImapCopyMessagesStrategy::messageListMessageAction(ImapStrategyContextBase *context)
{
    if (_messageCount < _listSize) {
        context->updateStatus( QObject::tr("Copying %1 / %2").arg(_messageCount + 1).arg(_listSize) );
    }

    copyNextMessage(context);
}

void ImapCopyMessagesStrategy::messageListFolderAction(ImapStrategyContextBase *context)
{
    if (_currentMailbox.id().isValid() && (_currentMailbox.id() == _destination.id())) {
        // This message is already in the destination folder - we need to append a new copy
        // after closing the destination folder
        context->protocol().sendClose();
    } else {
        ImapFetchSelectedMessagesStrategy::messageListFolderAction(context);
    }
}

void ImapCopyMessagesStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    if (_transferState == Search) {
        // Move on to the next message set
        selectMessageSet(context);
    } else {
        // We have copied all the messages - now we need to retrieve the copies
        _transferState = Search;
        context->protocol().sendSelect(_destination);
    }
}

void ImapCopyMessagesStrategy::messageCopied(ImapStrategyContextBase *context, const QString &copiedUid, const QString &createdUid)
{
    if (!createdUid.isEmpty()) {
        _createdUids.append(createdUid);

        _sourceUid[createdUid] = copiedUid;
        _sourceUids.removeAll(copiedUid);
    }

    ImapFetchSelectedMessagesStrategy::messageCopied(context, copiedUid, createdUid);
}

void ImapCopyMessagesStrategy::messageCreated(ImapStrategyContextBase *context, const QMailMessageId &id, const QString &createdUid)
{
    if (!createdUid.isEmpty()) {
        _createdUids.append(createdUid);

        QString pseudoUid("id:" + QString::number(id.toULongLong()));
        _sourceUid[createdUid] = pseudoUid;
        _sourceUids.removeAll(pseudoUid);
    }

    ImapFetchSelectedMessagesStrategy::messageCreated(context, id, createdUid);
}

void ImapCopyMessagesStrategy::messageFetched(ImapStrategyContextBase *context, QMailMessage &message)
{ 
    QString sourceUid = copiedMessageFetched(context, message);

    ImapFetchSelectedMessagesStrategy::messageFetched(context, message);

    if (!sourceUid.isEmpty()) {
        // We're now completed with the source message also
        context->completedMessageAction(sourceUid);
    }
}

QString ImapCopyMessagesStrategy::copiedMessageFetched(ImapStrategyContextBase *context, QMailMessage &message)
{
    // See if we can update the status of the copied message from the source message
    QString sourceUid = _sourceUid[message.serverUid()];
    if (sourceUid.isEmpty()) {
        if (_sourceIndex < _sourceUids.count()) {
            sourceUid = _sourceUids.at(_sourceIndex);
            _sourceIndex++;
        }
    }

    if (!sourceUid.isEmpty()) {
        QMailMessage source;
        if (sourceUid.startsWith("id:")) {
            source = QMailMessage(QMailMessageId(sourceUid.mid(3).toULongLong()));
        } else {
            source = QMailMessage(sourceUid, context->config().id());
        }

        if (source.id().isValid()) {
            updateCopiedMessage(context, message, source);
        } else {
            _error = true;
            qWarning() << "Unable to update message from UID:" << sourceUid << "to copy:" << message.serverUid();
        }

        context->completedMessageCopy(message, source);
    }

    return sourceUid;
}

void ImapCopyMessagesStrategy::updateCopiedMessage(ImapStrategyContextBase *, QMailMessage &message, const QMailMessage &source)
{
    if ((source.status() & QMailMessage::New) == 0) {
        message.setStatus(QMailMessage::New, false);
    }

    if((source.status() & QMailMessage::Trash) == 0)
        message.setStatus(QMailMessage::Trash,false);

    if (source.status() & QMailMessage::Read) {
        message.setStatus(QMailMessage::Read, true);
    }
}

void ImapCopyMessagesStrategy::copyNextMessage(ImapStrategyContextBase *context)
{
    if (selectNextMessageSequence(context, 1)) {
        const QString &messageUid(_messageUids.first());
        ++_messageCount;

        _transferState = Copy;

        if (messageUid.startsWith("id:")) {
            // This message does not exist on the server - append it
            QMailMessageId id(messageUid.mid(3).toULongLong());
            context->protocol().sendAppend(_destination, id);
        } else if (!context->mailbox().id.isValid()) {
            // This message is in the destination folder, which we have closed
            QMailMessageMetaData metaData(messageUid, context->config().id());
            context->protocol().sendAppend(_destination, metaData.id());

            // The existing message should be removed once we have appended the new message
            _obsoleteDestinationUids.append(ImapProtocol::uid(messageUid));
        } else {
            // Copy this message from its current location to the destination
            context->protocol().sendUidCopy(ImapProtocol::uid(messageUid), _destination);
        }

        _sourceUids.append(messageUid);
    }
}

void ImapCopyMessagesStrategy::removeObsoleteUids(ImapStrategyContextBase *context)
{
    if (!_obsoleteDestinationUids.isEmpty()) {
        context->protocol().sendUidStore(MFlag_Deleted, true, IntegerRegion(_obsoleteDestinationUids).toString());
        _obsoleteDestinationUids.clear();
    } else {
        fetchNextCopy(context);
    }
}

void ImapCopyMessagesStrategy::fetchNextCopy(ImapStrategyContextBase *context)
{
    if (!_createdUids.isEmpty()) {
        QString firstUid = ImapProtocol::uid(_createdUids.takeFirst());
        context->protocol().sendUidFetch(MetaDataFetchFlags, firstUid);
    } else {
        messageListCompleted(context);
    }
}

void ImapCopyMessagesStrategy::selectMessageSet(ImapStrategyContextBase *context)
{
    if (!_messageSets.isEmpty()) {
        const QPair<QMailMessageIdList, QMailFolderId> &set(_messageSets.first());

        selectedMailsAppend(set.first);
        resetMessageListTraversal();

        _destination = QMailFolder(set.second);

        _messageSets.takeFirst();

        _transferState = Init;
        _obsoleteDestinationUids.clear();

        // We need to select the destination folder to reset the Recent list
        if (_destination.id() == context->mailbox().id) {
            // We already have the appropriate mailbox selected
            handleSelect(context);
        } else {
            context->protocol().sendSelect(_destination);
        }
    } else {
        // Nothing more to do
        ImapFetchSelectedMessagesStrategy::messageListCompleted(context);
    }
}


/* A strategy to move selected messages.
*/
// Note: the move strategy is:
// 1. Select the destination folder, resetting Recent
// 2. Copy each specified message from its current folder to the destination, recording the local ID
// 3. Mark each specified message as deleted
// 4. Close each modified folder to expunge the marked messages
// 5. Update the status for each modified folder
// 6. Select the destination folder
// 7. Search for recent messages
// 8. Retrieve metadata only for found messages
// 9. When the metadata for a copy is fetched, move the local body of the source message into the copy

void ImapMoveMessagesStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_Examine:
        {
            handleExamine(context);
            break;
        }
        
        default:
        {
            ImapCopyMessagesStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapMoveMessagesStrategy::handleUidCopy(ImapStrategyContextBase *context)
{
    // Mark the copied message(s) as deleted
    context->protocol().sendUidStore(MFlag_Deleted, true, numericUidSequence(_messageUids));
}

void ImapMoveMessagesStrategy::handleUidStore(ImapStrategyContextBase *context)
{
    if ((_transferState == Copy) || (_transferState == Complete)) {
        _lastMailbox = _currentMailbox;
        messageListMessageAction(context);
    } else {
        ImapCopyMessagesStrategy::handleUidStore(context);
    }
}

void ImapMoveMessagesStrategy::handleClose(ImapStrategyContextBase *context)
{
    if (_transferState == Copy) {
        context->protocol().sendExamine(_lastMailbox);
        _lastMailbox = QMailFolder();
    } else {
        ImapCopyMessagesStrategy::handleClose(context);
    }
}

void ImapMoveMessagesStrategy::handleExamine(ImapStrategyContextBase *context)
{
    // Select the next folder
    ImapCopyMessagesStrategy::messageListFolderAction(context);
}

void ImapMoveMessagesStrategy::handleUidFetch(ImapStrategyContextBase *context)
{
    // See if we can move the body of the source message to this message
    fetchNextCopy(context);
}

void ImapMoveMessagesStrategy::messageListFolderAction(ImapStrategyContextBase *context)
{
    // If we're already in a mailbox, we need to close it to expunge the messages
    if ((context->mailbox().isSelected()) && (_lastMailbox.id().isValid())) {
        context->protocol().sendClose();
    } else {
        ImapCopyMessagesStrategy::messageListFolderAction(context);
    }
}

void ImapMoveMessagesStrategy::messageListMessageAction(ImapStrategyContextBase *context)
{
    if (_messageCount < _listSize) {
        context->updateStatus( QObject::tr("Moving %1 / %2").arg(_messageCount + 1).arg(_listSize) );
    }

    copyNextMessage(context);
}

void ImapMoveMessagesStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    if (_transferState == Search) {
        // We don't need to keep the source messages any longer
        QStringList allSourceUids;
        foreach (const QString &uid, (_sourceUids + _sourceUid.values())) {
            if (!uid.startsWith("id:")) {
                allSourceUids.append(uid);
            } else {
                // TODO: Are these messages removed?
            }
        }

        if (!allSourceUids.isEmpty()) {
            QMailMessageKey uidKey(QMailMessageKey::serverUid(allSourceUids));
            QMailMessageKey accountKey(QMailMessageKey::parentAccountId(context->config().id()));

            if (!QMailStore::instance()->removeMessages(accountKey & uidKey, QMailStore::NoRemovalRecord)) {
                _error = true;
                qWarning() << "Unable to remove message for account:" << context->config().id() << "UIDs:" << allSourceUids;
            }
        }
    }

    ImapCopyMessagesStrategy::messageListCompleted(context);
}

void ImapMoveMessagesStrategy::updateCopiedMessage(ImapStrategyContextBase *context, QMailMessage &message, const QMailMessage &source)
{ 
    ImapCopyMessagesStrategy::updateCopiedMessage(context, message, source);

    // Move the content of the source message to the new message
    if (!transferMessageData(message, source)) {
        _error = true;
        qWarning() << "Unable to transfer message data";
        return;
    }

    message.setPreviousParentFolderId(QMailFolderId());
    if (source.serverUid().isEmpty()) {
        // This message has been moved to the external server - delete the local copy
        if (!QMailStore::instance()->removeMessages(QMailMessageKey::id(source.id()), QMailStore::NoRemovalRecord)) {
            _error = true;
            qWarning() << "Unable to remove moved message:" << source.id();
        }
    }
}


/* A strategy to make selected messages available from the external server.
*/
// 1. Select and then close the destination folder, resetting the Recent flag
// 2. Append each specified message to the destination folder
// 3. Select the destination folder
// 4. Search for recent messages 
// 5. Replace the original local message with the remote one
// 6. Generate authorized URLs for each message

void ImapExternalizeMessagesStrategy::appendMessageSet(const QMailMessageIdList &messageIds, const QMailFolderId& folderId)
{
    if (folderId.isValid()) {
        ImapCopyMessagesStrategy::appendMessageSet(messageIds, folderId);
    } else {
        // These messages can't be externalised - just remove the flag to proceed as normal
        QMailMessageKey idsKey(QMailMessageKey::id(messageIds));
        if (!QMailStore::instance()->updateMessagesMetaData(idsKey, QMailMessage::TransmitFromExternal, false)) {
            _error = true;
            qWarning() << "Unable to update message metadata to remove transmit from external flag";
        }
    }
}

void ImapExternalizeMessagesStrategy::newConnection(ImapStrategyContextBase *context)
{
    _urlIds.clear();

    if (!_messageSets.isEmpty()) {
        ImapCopyMessagesStrategy::newConnection(context);
    } else {
        // Nothing to do
        context->operationCompleted();
    }
}

void ImapExternalizeMessagesStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_GenUrlAuth:
        {
            handleGenUrlAuth(context);
            break;
        }
        
        default:
        {
            ImapCopyMessagesStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapExternalizeMessagesStrategy::handleGenUrlAuth(ImapStrategyContextBase *context)
{
    // We're finished with the previous location
    _urlIds.removeFirst();

    resolveNextMessage(context);
}

void ImapExternalizeMessagesStrategy::messageFetched(ImapStrategyContextBase *context, QMailMessage &message)
{ 
    copiedMessageFetched(context, message);

    ImapFetchSelectedMessagesStrategy::messageFetched(context, message);

    _urlIds.append(message.id());
}

void ImapExternalizeMessagesStrategy::urlAuthorized(ImapStrategyContextBase *, const QString &url)
{
    const QMailMessageId &id(_urlIds.first());

    // We now have an authorized URL for this message
    QMailMessage message(id);
    message.setExternalLocationReference(url);

    if (!QMailStore::instance()->updateMessage(&message)) {
        _error = true;
        qWarning() << "Unable to update message for account:" << message.parentAccountId();
    }
}

void ImapExternalizeMessagesStrategy::updateCopiedMessage(ImapStrategyContextBase *context, QMailMessage &message, const QMailMessage &source)
{ 
    // Update the source message with the new 
    ImapCopyMessagesStrategy::updateCopiedMessage(context, message, source);

    // Move the content of the source message to the new message
    if (!transferMessageData(message, source)) {
        _error = true;
        qWarning() << "Unable to transfer message data";
        return;
    }

    // Replace the source message with the new message
    message.setId(source.id());

    if (source.status() & QMailMessage::Outbox) {
        message.setStatus(QMailMessage::Outbox, true);
    }
    if (source.status() & QMailMessage::TransmitFromExternal) {
        message.setStatus(QMailMessage::TransmitFromExternal, true);
    }
}

void ImapExternalizeMessagesStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    resolveNextMessage(context);
}

void ImapExternalizeMessagesStrategy::resolveNextMessage(ImapStrategyContextBase *context)
{
    if (!_urlIds.isEmpty()) {
        const QMailMessageId &id(_urlIds.first());

        // Generate an authorized URL for this message
        QMailMessagePart::Location location;
        location.setContainingMessageId(id);
        context->protocol().sendGenUrlAuth(location, false);
    } else {
        ImapCopyMessagesStrategy::messageListCompleted(context);
    }
}


/* A strategy to flag selected messages.
*/
void ImapFlagMessagesStrategy::clearSelection()
{
    _setMask = 0;
    _unsetMask = 0;
    _outstandingStores = 0;

    ImapFetchSelectedMessagesStrategy::clearSelection();
}

void ImapFlagMessagesStrategy::setMessageFlags(MessageFlags flags, bool set)
{
    if (set) {
        _setMask |= flags;
    } else {
        _unsetMask |= flags;
    }
}

void ImapFlagMessagesStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_UIDStore:
        {
            handleUidStore(context);
            break;
        }
        
        default:
        {
            ImapFetchSelectedMessagesStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapFlagMessagesStrategy::handleUidStore(ImapStrategyContextBase *context)
{
    --_outstandingStores;
    if (_outstandingStores == 0) {
        messageListMessageAction(context);
    }
}

void ImapFlagMessagesStrategy::messageListMessageAction(ImapStrategyContextBase *context)
{
    const int batchSize = 1000;
    if (selectNextMessageSequence(context, batchSize)) {
        QString uidSequence(numericUidSequence(_messageUids));
        if (_setMask) {
            context->protocol().sendUidStore(_setMask, true, uidSequence);
            ++_outstandingStores;
        }
        if (_unsetMask) {
            context->protocol().sendUidStore(_unsetMask, false, uidSequence);
            ++_outstandingStores;
        }
    }
}


/* A strategy to delete selected messages.
*/
void ImapDeleteMessagesStrategy::setLocalMessageRemoval(bool removal)
{
    _removal = removal;

    setMessageFlags(MFlag_Deleted, true);
}

void ImapDeleteMessagesStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_Close:
        {
            handleClose(context);
            break;
        }
        
        case IMAP_Examine:
        {
            handleExamine(context);
            break;
        }
        
        default:
        {
            ImapFlagMessagesStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapDeleteMessagesStrategy::handleUidStore(ImapStrategyContextBase *context)
{
    if (_outstandingStores == 1) {
        // Add the stored UIDs to our list
        _storedList += _messageUids;
        _lastMailbox = _currentMailbox;
    }

    ImapFlagMessagesStrategy::handleUidStore(context);
}

void ImapDeleteMessagesStrategy::handleClose(ImapStrategyContextBase *context)
{
    if (_removal) {
        // All messages in the stored list are now expunged - delete the local copies
        QMailMessageKey accountKey(QMailMessageKey::parentAccountId(context->config().id()));
        QMailMessageKey uidKey(QMailMessageKey::serverUid(_storedList));

        if (!QMailStore::instance()->removeMessages(accountKey & uidKey, QMailStore::NoRemovalRecord)) {
            _error = true;
            qWarning() << "Unable to remove message for account:" << context->config().id() << "UIDs:" << _storedList;
        }
    }

    // We need to examine the closed mailbox to find the new server counts
    context->protocol().sendExamine(_lastMailbox);
    _lastMailbox = QMailFolder();
}

void ImapDeleteMessagesStrategy::handleExamine(ImapStrategyContextBase *context)
{
    // Select the next folder
    ImapFlagMessagesStrategy::messageListFolderAction(context);
}

void ImapDeleteMessagesStrategy::messageListFolderAction(ImapStrategyContextBase *context)
{
    // If we're already in a mailbox, we need to close it to expunge the messages
    if ((context->mailbox().isSelected()) && (_lastMailbox.id().isValid())) {
        context->protocol().sendClose();
    } else {
        // Select the next folder, and clear the stored list
        _storedList.clear();
        ImapFlagMessagesStrategy::messageListFolderAction(context);
    }
}

void ImapDeleteMessagesStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    // If we're already in a mailbox, we need to close it to expunge the messages
    if ((context->mailbox().isSelected()) && (_lastMailbox.id().isValid())) {
        context->protocol().sendClose();
    } else {
        ImapFlagMessagesStrategy::messageListCompleted(context);
    }
}


ImapStrategyContext::ImapStrategyContext(ImapClient *client)
  : ImapStrategyContextBase(client), 
    _strategy(0) 
{
}

