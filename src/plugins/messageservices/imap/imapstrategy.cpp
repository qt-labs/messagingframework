/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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

// TODO: Use pipelined commands where appropriate - where error handling remains 
// sensible and it isn't too difficult to tell which commands have/haven't completed...

static const int MetaDataFetchFlags = F_Uid | F_Date | F_Rfc822_Size | F_Rfc822_Header | F_BodyStructure;
static const int ContentFetchFlags = F_Uid | F_Rfc822_Size | F_Rfc822;

static QString stripFolderPrefix(const QString &str)
{
    QString result;
    int index;
    if ((index = str.lastIndexOf('|')) != -1)
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

static void updateMessagesMetaData(ImapStrategyContextBase *context, 
                                   const QMailMessageKey &storedKey, 
                                   const QMailMessageKey &unseenKey, 
                                   const QMailMessageKey &seenKey,
                                   const QMailMessageKey &unreadElsewhereKey)
{
    QMailMessageKey reportedKey(seenKey | unseenKey);

    // Mark as deleted any messages that the server does not report
    QMailMessageKey nonexistentKey(storedKey & ~reportedKey);
    if (!QMailStore::instance()->updateMessagesMetaData(nonexistentKey, QMailMessage::Removed, true)) {
        qWarning() << "Unable to update removed message metadata for account:" << context->config().id();
    }
    
    foreach (const QMailMessageMetaData& r, QMailStore::instance()->messagesMetaData(nonexistentKey, QMailMessageKey::ServerUid))  {
        const QString &uid(r.serverUid()); 
        // We might have a deletion record for this UID
        if (!QMailStore::instance()->purgeMessageRemovalRecords(context->config().id(), QStringList() << uid)) {
            qWarning() << "Unable to purge message records for account:" << context->config().id();
        }
        context->completedMessageAction(uid);
    }

    // Compensate for MS exchange temporarily failing to report existence of messages
    QMailMessageKey existentUidKey(storedKey & reportedKey);
    QMailMessageKey removedUidKey(QMailMessageKey::status(QMailMessage::Removed, QMailDataComparator::Includes));
    QMailMessageKey onServerButRemovedInStore(removedUidKey & existentUidKey);
    if (!QMailStore::instance()->updateMessagesMetaData(onServerButRemovedInStore, QMailMessage::Removed, false)) {
        qWarning() << "Unable to update unremoved message metadata for account:" << context->config().id();
    }

    // Update any messages that are reported as read elsewhere, that previously were not
    if (!QMailStore::instance()->updateMessagesMetaData(seenKey & unreadElsewhereKey, QMailMessage::ReadElsewhere, true)) {
        qWarning() << "Unable to update read message metadata for account:" << context->config().id();
    }
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

/* A basic strategy to achieve an authenticated state with the server,
   and to provide default responses to IMAP command completion notifications,
*/
void ImapStrategy::newConnection(ImapStrategyContextBase *context)
{
    _transferState = Init;

    initialAction(context);
}

void ImapStrategy::initialAction(ImapStrategyContextBase *context)
{
    if (context->protocol().loggingOut())
        context->protocol().close();
    if (context->protocol().inUse()) {
        // We have effectively just completed authenticating
        transition(context, IMAP_Login, OpOk);
    } else {
        ImapConfiguration imapCfg(context->config());
        context->protocol().open(imapCfg);
    }
}

void ImapStrategy::mailboxListed(ImapStrategyContextBase *, QMailFolder& folder, const QString &flags)
{
    if (!folder.id().isValid()) {
        if (!QMailStore::instance()->addFolder(&folder)) {
            qWarning() << "Unable to add folder for account:" << folder.parentAccountId() << "path:" << folder.path();
        }
    }

    Q_UNUSED(flags)
}

void ImapStrategy::messageFetched(ImapStrategyContextBase *context, QMailMessage &message)
{ 
    // Store this message to the mail store
    if (message.id().isValid()) {
        if (!QMailStore::instance()->updateMessage(&message)) {
            qWarning() << "Unable to add message for account:" << message.parentAccountId() << "UID:" << message.serverUid();
            return;
        }
    } else {
        if (!QMailStore::instance()->addMessage(&message)) {
            qWarning() << "Unable to add message for account:" << message.parentAccountId() << "UID:" << message.serverUid();
            return;
        }

        context->folderModified(message.parentFolderId());
    }

    context->completedMessageAction(message.serverUid());
}

void ImapStrategy::dataFetched(ImapStrategyContextBase *context, QMailMessage &message, const QString &uid, const QString &section)
{
    // Store the updated message
    if (!QMailStore::instance()->updateMessage(&message)) {
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

void ImapStrategy::downloadSize(ImapStrategyContextBase *context, const QString &uid, int length)
{
    Q_UNUSED(context)
    Q_UNUSED(uid)
    Q_UNUSED(length)
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

    const QMailFolderId trashFolderId(QMailFolder::TrashFolder);

    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentFolderId | QMailMessageKey::PreviousParentFolderId | QMailMessageKey::ServerUid);
    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(ids), props)) {
        // TODO - order messages within each folder by size ascending, or most recent first?
        QMailFolderId parentFolderId(metaData.parentFolderId() == trashFolderId ? metaData.previousParentFolderId() : metaData.parentFolderId());
        bool ok;
        uint serverUid(stripFolderPrefix(metaData.serverUid()).toUInt(&ok));
        _selectionMap[parentFolderId].insert(serverUid, SectionProperties());
    }

    _folderItr = _selectionMap.begin();
    _selectionItr = _folderItr.value().begin();
}

void ImapMessageListStrategy::selectedSectionsAppend(const QMailMessagePart::Location &location)
{
    QMailMessageMetaData metaData(location.containingMessageId());
    if (metaData.id().isValid()) {
        SectionProperties sectionProperties(location);
        bool ok;
        uint serverUid(stripFolderPrefix(metaData.serverUid()).toUInt(&ok));
        _selectionMap[metaData.parentFolderId()].insert(serverUid, sectionProperties);
    }

    _folderItr = _selectionMap.begin();
    _selectionItr = _folderItr.value().begin();
}

void ImapMessageListStrategy::newConnection(ImapStrategyContextBase *context)
{
    _currentMailbox = QMailFolder();

    ImapStrategy::newConnection(context);
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
        
        case IMAP_Logout:
        {
            break;
        }
        
        default:
        {
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

bool ImapMessageListStrategy::computeStartEndPartRange(ImapStrategyContextBase *context)
{
    if (!_retrieveUid.isEmpty()) {
        QMailMessage mail(_retrieveUid, context->config().id());
        if (mail.id().isValid()) {
            _sectionStart = 0;
            if (_msgSection.isValid()) {
                const QMailMessagePart &part(mail.partAt(_msgSection));
                if (part.hasBody()) {
                    _sectionStart = part.body().length();
                }
            } else {
                if (mail.hasBody()) {
                    _sectionStart = mail.body().length();
                }
            }
            return true;
        }
    }
    return false;
}

bool ImapMessageListStrategy::selectNextMessageSequence(ImapStrategyContextBase *context, int maximum)
{
    QMailFolderId mailboxId;
    QString mailboxIdStr;
    QStringList uidList;
    QMailMessagePart::Location location;
    int minimum = SectionProperties::All;

    if (_folderItr == _selectionMap.end()) {
        messageListCompleted(context);
        return false;
    }
        
    FolderMap::ConstIterator selectionEnd = _folderItr.value().end();
    while (_selectionItr == selectionEnd) {
        ++_folderItr;
        if (_folderItr == _selectionMap.end())
            break;
        _selectionItr = _folderItr.value().begin();
        selectionEnd = _folderItr.value().end();
    }

    if ((_folderItr == _selectionMap.end()) || !_folderItr.key().isValid()) {
        _currentMailbox = QMailFolder();
        _selectionMap.clear();
        messageListFolderAction(context);
        return false;
    }

    _transferState = Complete;
    mailboxId = _folderItr.key();
    if (mailboxId != _currentMailbox.id()) {
        _currentMailbox = QMailFolder(mailboxId);
        messageListFolderAction(context);
        return false;
    }
    mailboxIdStr = QString::number(mailboxId.toULongLong()) + '|';

    //TODO Leave parts to last to reduce roundtrips. Get all parts in one message in one roundtrip.
    while ((_selectionItr != selectionEnd) 
           && (uidList.count() < maximum)
           && (!location.isValid())
           && (minimum == SectionProperties::All)) {
        uidList.append(mailboxIdStr + QString::number(_selectionItr.key()));
        location = _selectionItr.value()._location;
        minimum = _selectionItr.value()._minimum;

        ++_selectionItr;
        if (_selectionItr != selectionEnd) {
            if (_selectionItr.value()._location.isValid()
                || (_selectionItr.value()._minimum != SectionProperties::All))
                break;  // _msgSection.isValid() parts still require a roundtrip
        }
    }

    _retrieveUid = uidList.join("\n");
    _msgSection = location;
    _sectionStart = 0;
    _sectionEnd = SectionProperties::All;
    if (minimum != SectionProperties::All) {
        _sectionEnd = minimum - 1;
        if (!minimum || !computeStartEndPartRange(context)) {
            if (_sectionStart <= _sectionEnd) {
                qMailLog(Messaging) << "Could not complete part: invalid location or invalid uid";
            } // else already have retrieved minimum bytes, so nothing todo
            return selectNextMessageSequence(context, maximum);
        }
    }

    return true;
}

void ImapMessageListStrategy::messageListFolderAction(ImapStrategyContextBase *context)
{
    if (_currentMailbox.id().isValid()) {
        if (_currentMailbox.id() == context->mailbox().id) {
            // We already have the appropriate mailbox selected
            handleSelect(context);
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

    const QMailFolderId trashFolderId(QMailFolder::TrashFolder);

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
            // TODO - order messages within each folder by size ascending, or most recent first?
            QMailFolderId parentFolderId(metaData.parentFolderId() == trashFolderId ? metaData.previousParentFolderId() : metaData.parentFolderId());
            bool ok;
            uint serverUid(stripFolderPrefix(metaData.serverUid()).toUInt(&ok));
            _selectionMap[parentFolderId].insert(serverUid, SectionProperties());

            uint size = metaData.indicativeSize();
            uint bytes = metaData.size();

            _retrievalSize.insert(metaData.serverUid(), qMakePair(qMakePair(size, bytes), 0u));
            _totalRetrievalSize += size;
        }
    }

    _folderItr = _selectionMap.begin();
    _selectionItr = _folderItr.value().begin();

    // Report the total size we will retrieve
    _progressRetrievalSize = 0;
}

void ImapFetchSelectedMessagesStrategy::selectedSectionsAppend(const QMailMessagePart::Location &location, int minimum)
{
    _listSize += 1;

    QMailMessageMetaData metaData(location.containingMessageId());
    if (metaData.id().isValid()) {
        SectionProperties sectionProperties(location, minimum);
        bool ok;
        uint serverUid(stripFolderPrefix(metaData.serverUid()).toUInt(&ok));
        _selectionMap[metaData.parentFolderId()].insert(serverUid, sectionProperties);

        uint size = metaData.indicativeSize();
        uint bytes = metaData.size();
        if (minimum > 0)
            size = bytes = minimum;

        _retrievalSize.insert(metaData.serverUid(), qMakePair(qMakePair(size, bytes), 0u));
        _totalRetrievalSize += size;
    }

    _folderItr = _selectionMap.begin();
    _selectionItr = _folderItr.value().begin();
}

void ImapFetchSelectedMessagesStrategy::newConnection(ImapStrategyContextBase *context)
{
    _progressRetrievalSize = 0;
    _messageCount = 0;

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
    messageListMessageAction(context);
}

void ImapFetchSelectedMessagesStrategy::messageListMessageAction(ImapStrategyContextBase *context)
{
    if (selectNextMessageSequence(context)) {
        QStringList uids;
        foreach (const QString &msgUid, _retrieveUid.split("\n"))
            uids.append(ImapProtocol::uid(msgUid));

        _messageCountIncremental = _messageCount;
        context->updateStatus(QObject::tr("Completing %1 / %2").arg(_messageCount + 1).arg(_listSize));
        _messageCount += uids.count();

        QString msgSectionStr;
        if (_msgSection.isValid())
            msgSectionStr = _msgSection.toString(false);
        if (_msgSection.isValid() || (_sectionEnd != SectionProperties::All)) {
            context->protocol().sendUidFetchSection(IntegerRegion(uids).toString(),
                                                    msgSectionStr,
                                                    _sectionStart, 
                                                    _sectionEnd);
        } else {
            context->protocol().sendUidFetch(ContentFetchFlags, IntegerRegion(uids).toString());
        }
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

        int count = qMin(++_messageCountIncremental + 1, _listSize);
        context->updateStatus(QObject::tr("Completing %1 / %2").arg(count).arg(_listSize));

        _retrievalSize.erase(it);
    }
}


/* A strategy that provides an interface for processing a set of folders.
*/
void ImapFolderListStrategy::selectedFoldersAppend(const QMailFolderIdList& ids) 
{
    _mailboxIds += ids;
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

        // We need the UID of the most recent message we have previously discovered in this folder
        QMailMessageKey key(QMailMessageKey::parentFolderId(_currentMailbox.id()));
        QMailMessageSortKey sortKey(QMailMessageSortKey::id(Qt::DescendingOrder));

        QMailMessageIdList messageIds(QMailStore::instance()->queryMessages(key, sortKey));
        if (!messageIds.isEmpty()) {
            // Find this message's MSN
            QMailMessageMetaData message(messageIds.first());
            context->protocol().sendSearch(0, QString("UID ") + ImapProtocol::uid(message.serverUid()));
        } else {
            // No messages - nothing to discover...
            handleSearch(context);
        }
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

        if (_folderStatus.contains(folderId)) {
            FolderStatus folderState = _folderStatus[folderId];
            if (folderState & NoSelect) {
                // We can't select this folder
                continue;
            }
        }

        // Process this folder
        _currentMailbox = QMailFolder(folderId);

        // Bypass any folder for which synchronization is disabled
        if (_currentMailbox.status() & QMailFolder::SynchronizationEnabled)
            return true;
    }

    return false;
}

void ImapFolderListStrategy::processFolder(ImapStrategyContextBase *context)
{
    if (_folderStatus.contains(_currentMailbox.id())) {
        // We already have the status for this folder - select it
        context->protocol().sendSelect(_currentMailbox);
    } else {
        // List the contents of the parent of this folder, to see if we can select it
        QMailFolderId parentFolderId(_currentMailbox.parentFolderId());
        if (parentFolderId.isValid()) {
            context->protocol().sendList(QMailFolder(parentFolderId), "%");
        } else {
            QMailFolder root;
            ImapConfiguration imapCfg(context->config());
            if (!imapCfg.baseFolder().isEmpty()) {
                root.setPath(imapCfg.baseFolder());
            }

            context->protocol().sendList(root, "%");
        }
    }
}

void ImapFolderListStrategy::folderListCompleted(ImapStrategyContextBase *context)
{
    // We have retrieved all the folders - process any messages
    messageListMessageAction(context);
}

void ImapFolderListStrategy::mailboxListed(ImapStrategyContextBase *context, QMailFolder &folder, const QString &flags)
{
    ImapStrategy::mailboxListed(context, folder, flags);

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

void ImapFolderListStrategy::updateUndiscoveredCount(ImapStrategyContextBase *context)
{
    // We should have the MSN of the our most recent message
    uint msn = 0;
    if (!context->mailbox().msnList.isEmpty()) {
        msn = context->mailbox().msnList.first();
    }

    // The difference between the server count and the MSN of our message should yield the undiscovered count
    // Note: MSN is 1-based, count is 0-based.
    QMailFolder folder(_currentMailbox.id());
    int undiscovered = folder.serverCount() - msn;
    if ((undiscovered >= 0) && (uint(undiscovered) != folder.serverUndiscoveredCount())) {
        folder.setServerUndiscoveredCount(undiscovered);

        if (!QMailStore::instance()->updateFolder(&folder)) {
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
        // We're retrieving message metadata
        fetchNextMailPreview(context);
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
        fetchNextMailPreview(context);
    } else if (_transferState == Complete) {    //getting complete messages
        messageListMessageAction(context);
    }
}

void ImapSynchronizeBaseStrategy::processUidSearchResults(ImapStrategyContextBase *)
{
    qWarning() << "ImapSynchronizeBaseStrategy::processUidSearchResults: Unexpected location!";
}

void ImapSynchronizeBaseStrategy::previewDiscoveredMessages(ImapStrategyContextBase *context)
{
    // Process our list of all messages to be retrieved for each mailbox
    _total = 0;
    QList<QPair<QMailFolderId, QStringList> >::const_iterator it = _retrieveUids.begin(), end = _retrieveUids.end();
    for ( ; it != end; ++it)
        _total += it->second.count();

    context->updateStatus(QObject::tr("Previewing", "Previewing <number of messages>") + QChar(' ') + QString::number(_total));

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
        _currentMailbox = QMailFolder();
        _newUids = QStringList();
        return false;
    }

    // In preview mode, select the mailboxes where retrievable messages are located
    QPair<QMailFolderId, QStringList> next = _retrieveUids.takeFirst();
    _currentMailbox = QMailFolder(next.first);
    _newUids = next.second;

    FolderStatus folderState = _folderStatus[_currentMailbox.id()];
    if (folderState & NoSelect) {
        // Bypass the select and UID search, and go directly to the search result handler
        processUidSearchResults(context);
    } else {
        if (_currentMailbox.id() == context->mailbox().id) {
            // We already have the appropriate mailbox selected
            handleSelect(context);
        } else {
            if (_transferState == List) {
                QString status = QObject::tr("Checking", "Checking <mailbox name>") + QChar(' ') + _currentMailbox.displayName();
                context->updateStatus( status );
            }

            context->protocol().sendSelect( _currentMailbox );
        }
    }

    return true;
}

void ImapSynchronizeBaseStrategy::fetchNextMailPreview(ImapStrategyContextBase *context)
{
    if (_newUids.count() > 0) {
        QStringList uidList;
        foreach(const QString &s, _newUids.mid(0, DefaultBatchSize)) {
            uidList << ImapProtocol::uid(s);
        }

        context->protocol().sendUidFetch(MetaDataFetchFlags, IntegerRegion(uidList).toString());
        _newUids = _newUids.mid(uidList.count());
    } else if (!selectNextPreviewFolder(context)) {
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

                messageListMessageAction(context);
            } else {
                // No messages to be completed
                messageListCompleted(context);
            }
        }
    }
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
                    if (bytesLeft > disposition.size()) {
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
    } else {
        folderId = context->client()->mailboxId(imapCfg.baseFolder());
    }

    _transferState = List;

    if (folderId.isValid()) {
        // Begin processing with the specified base folder
        selectedFoldersAppend(QMailFolderIdList() << folderId);
        ImapSynchronizeBaseStrategy::handleLogin(context);
    } else {
        // We need to search for folders at the account root
        QMailFolder root;
        if (!imapCfg.baseFolder().isEmpty()) {
            root.setPath(imapCfg.baseFolder());
        }

        context->protocol().sendList(root, "%");
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

void ImapRetrieveFolderListStrategy::processNextFolder(ImapStrategyContextBase *context)
{
    if (nextFolder()) {
        processFolder(context);
        return;
    }

    // We should have discovered all available mailboxes now
    _mailboxList = context->client()->mailboxIds();

    folderListCompleted(context);
}

void ImapRetrieveFolderListStrategy::folderListCompleted(ImapStrategyContextBase *context)
{
    removeDeletedMailboxes(context);

    // We have retrieved all the folders - process any messages
    messageListMessageAction(context);
}

void ImapRetrieveFolderListStrategy::mailboxListed(ImapStrategyContextBase *context, QMailFolder &folder, const QString &flags)
{
    ImapSynchronizeBaseStrategy::mailboxListed(context, folder, flags);

    _mailboxPaths.append(folder.path());

    if (_descending) {
        if (folder.id() != _currentMailbox.id()) {
            // We need to list this folder's contents, too
            selectedFoldersAppend(QMailFolderIdList() << folder.id());
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
    switch(_searchState)
    {
    case Seen:
    {
        _seenUids = context->mailbox().uidList;

        _searchState = Unseen;
        context->protocol().sendUidSearch(MFlag_Unseen);
        break;
    }
    case Unseen:
    {
        _unseenUids = context->mailbox().uidList;
        if ((_unseenUids.count() + _seenUids.count()) == context->mailbox().exists) {
            // We have a consistent set of search results
            processUidSearchResults(context);
        } else {
            qMailLog(IMAP) << "Inconsistent UID SEARCH result using SEEN/UNSEEN; reverting to ALL";

            // Try doing a search for ALL messages
            _unseenUids.clear();
            _seenUids.clear();
            _searchState = All;
            context->protocol().sendUidSearch(MFlag_All);
        }
        break;
    }
    case All:
    {
        _unseenUids = context->mailbox().uidList;
        if (_unseenUids.count() != context->mailbox().exists) {
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
    if (!setNextSeen(context))
        if (!setNextDeleted(context))
            processNextFolder(context);
}

void ImapSynchronizeAllStrategy::handleExpunge(ImapStrategyContextBase *context)
{
    processNextFolder(context);
}

void ImapSynchronizeAllStrategy::folderListFolderAction(ImapStrategyContextBase *context)
{
    _seenUids = QStringList();
    _unseenUids = QStringList();
    _readUids = QStringList();
    _removedUids = QStringList();
    _expungeRequired = false;
    
    // Search for messages in the current mailbox
    _searchState = Seen;

    if (context->mailbox().exists > 0) {
        // Start by looking for previously-seen and unseen messages
        context->protocol().sendUidSearch(MFlag_Seen);
    } else {
        // No messages, so no need to perform search
        processUidSearchResults(context);
    }
}

void ImapSynchronizeAllStrategy::folderListCompleted(ImapStrategyContextBase *context)
{
    removeDeletedMailboxes(context);

    previewDiscoveredMessages(context);
}

void ImapSynchronizeAllStrategy::processUidSearchResults(ImapStrategyContextBase *context)
{
    QMailFolderId boxId = _currentMailbox.id();
    QMailMessageKey accountKey(QMailMessageKey::parentAccountId(context->config().id()));
    QMailFolder folder(boxId);
    
    // Folder min/max invalidated
    folder.removeCustomField("qmf-min-serveruid");
    folder.removeCustomField("qmf-max-serveruid");
    
    if ((_currentMailbox.status() & QMailFolder::SynchronizationEnabled) &&
        !(_currentMailbox.status() & QMailFolder::Synchronized)) {
        // We have just synchronized this folder
        folder.setStatus(QMailFolder::Synchronized, true);
    }
    if (!QMailStore::instance()->updateFolder(&folder)) {
        qWarning() << "Unable to update folder for account:" << context->config().id();
    }

    // Messages reported as being on the server
    QStringList reportedOnServerUids = _seenUids + _unseenUids;

    // Messages (with at least metadata) stored locally
    QMailMessageKey folderKey(context->client()->messagesKey(boxId) | context->client()->trashKey(boxId));
    QStringList deletedUids = context->client()->deletedMessages(boxId);
    QMailMessageKey storedKey((accountKey & folderKey) | QMailMessageKey::serverUid(deletedUids));

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
        QMailMessageKey unreadElsewhereKey(folderKey & accountKey & ~readStatusKey);
        QMailMessageKey unseenKey(QMailMessageKey::serverUid(_unseenUids));
        QMailMessageKey seenKey(QMailMessageKey::serverUid(_seenUids));
    
        // Only delete messages the server still has
        _removedUids = inFirstAndSecond(deletedUids, reportedOnServerUids);
        _expungeRequired = !_removedUids.isEmpty();

        if (_options & ImportChanges) {
            updateMessagesMetaData(context, storedKey, unseenKey, seenKey, unreadElsewhereKey);
        }

        // Update messages on the server that are still flagged as unseen but have been read locally
        QMailMessageKey readHereKey(folderKey 
                                    & accountKey
                                    & QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Includes)
                                    & QMailMessageKey::serverUid(_unseenUids));
        _readUids = context->client()->serverUids(readHereKey);

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
        QString msgUidl = _readUids.first();
        QString msg = QObject::tr("Marking message %1 read").arg(msgUidl);
        _readUids.removeAll( msgUidl );

        context->updateStatus( msg );
        context->protocol().sendUidStore(MFlag_Seen, ImapProtocol::uid(msgUidl));
        return true;
    }

    return false;
}

bool ImapSynchronizeAllStrategy::setNextDeleted(ImapStrategyContextBase *context)
{
    ImapConfiguration imapCfg(context->config());
    if (imapCfg.canDeleteMail()) {
        if (!_removedUids.isEmpty()) {
            QString msgUidl = _removedUids.first();
            QString msg = QObject::tr("Deleting message %1").arg(msgUidl);
            _removedUids.removeAll( msgUidl );

            context->updateStatus( msg );

            //remove records of deleted messages
            if (!QMailStore::instance()->purgeMessageRemovalRecords(context->config().id(), QStringList() << msgUidl)) {
                qWarning() << "Unable to purge message record for account:" << context->config().id();
            }

            context->protocol().sendUidStore(MFlag_Deleted, ImapProtocol::uid(msgUidl));
            return true;
        } else if (_expungeRequired) {
            // All messages flagged as deleted, expunge them
            context->protocol().sendExpunge();
            return true;
        }
    }

    return false;
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
   That is to export flag changes (unseen -> seen) and deletes.
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

    QMailMessageKey statusKey(QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Includes));
    statusKey &= ~QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Includes);

    _folderMessageUids.clear();

    ImapClient *c(context->client());
    foreach (const QMailFolderId &folderId, context->client()->mailboxIds()) {
        QMailMessageKey folderKey(c->messagesKey(folderId) | c->trashKey(folderId));

        // Find messages marked as read locally
        QStringList deletedUids;
        QStringList readUids = c->serverUids(folderKey & statusKey);

        if (imapCfg.canDeleteMail()) {
            // Also find messages deleted locally
            deletedUids = context->client()->deletedMessages(folderId);
        }

        if (!readUids.isEmpty() || !deletedUids.isEmpty())
            _folderMessageUids.insert(folderId, qMakePair(readUids, deletedUids));
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
        IntegerRegion clientRegion(stripFolderPrefix(_clientReadUids + _clientDeletedUids));
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

    QMap<QMailFolderId, QPair<QStringList, QStringList> >::iterator it = _folderMessageUids.begin();

    _currentMailbox = QMailFolder(it.key());
    _clientReadUids = it.value().first;
    _clientDeletedUids = it.value().second;

    _folderMessageUids.erase(it);
    return true;
}

void ImapExportUpdatesStrategy::processUidSearchResults(ImapStrategyContextBase *context)
{
    // Messages marked deleted locally that the server reports exists
    _removedUids = inFirstAndSecond(_clientDeletedUids, _serverReportedUids);
    _expungeRequired = !_removedUids.isEmpty();

    // Messages marked read locally that the server reports exists
    _readUids = inFirstAndSecond(_clientReadUids, _serverReportedUids);

    handleUidStore(context);
}


/* A strategy to update message flags for a list of messages.
   
   That is to detect changes to flags (unseen->seen) 
   and to detect deleted mails.
*/
void ImapUpdateMessagesFlagsStrategy::clearSelection()
{
    ImapFolderListStrategy::clearSelection();
    _monitoredFoldersIds.clear();
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
    _searchState = Seen;

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

void ImapUpdateMessagesFlagsStrategy::handleSelect(ImapStrategyContextBase *context)
{
    if (_transferState == List) {
        // We're searching mailboxes
        if (context->mailbox().exists > 0) {
            IntegerRegion clientRegion(stripFolderPrefix(_serverUids));
            _filter = clientRegion.toString();
            _searchState = Seen;

            // Start by looking for previously-seen and unseen messages
            context->protocol().sendUidSearch(MFlag_Seen, "UID " + _filter);
        } else {
            // No messages, so no need to perform search
            processUidSearchResults(context);
        }
    } else {
        ImapFolderListStrategy::handleSelect(context);
    }
}

void ImapUpdateMessagesFlagsStrategy::handleUidSearch(ImapStrategyContextBase *context)
{
    switch(_searchState)
    {
    case Seen:
    {
        _seenUids = context->mailbox().uidList;

        _searchState = Unseen;
        context->protocol().sendUidSearch(MFlag_Unseen, "UID " + _filter);
        break;
    }
    case Unseen:
    {
        _unseenUids = context->mailbox().uidList;
        processUidSearchResults(context);
        break;
    }
    default:
        qMailLog(IMAP) << "Unknown search status in transition";
        Q_ASSERT(0);
        messageListCompleted(context);
    }
}

bool ImapUpdateMessagesFlagsStrategy::nextFolder()
{
    if (_folderMessageUids.isEmpty()) {
        return false;
    }

    QMap<QMailFolderId, QStringList>::iterator it = _folderMessageUids.begin();

    _currentMailbox = QMailFolder(it.key());
    _serverUids = it.value();

    _folderMessageUids.erase(it);
    return true;
}

void ImapUpdateMessagesFlagsStrategy::processFolder(ImapStrategyContextBase *context)
{
    QMailFolderId folderId(_currentMailbox.id());
    if ((folderId != context->client()->mailboxId("INBOX")) && 
        !_monitoredFoldersIds.contains(folderId)) {
        _monitoredFoldersIds << folderId;
    }

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
    QMailMessageKey folderKey(context->client()->messagesKey(folderId) | context->client()->trashKey(folderId));
    QMailMessageKey unreadElsewhereKey(folderKey & accountKey & ~readStatusKey);
    
    updateMessagesMetaData(context, storedKey, unseenKey, seenKey, unreadElsewhereKey);

    processNextFolder(context);
}

void ImapUpdateMessagesFlagsStrategy::folderListCompleted(ImapStrategyContextBase *context)
{
    // Only allow monitoring of one folder other than the inbox
    if (_monitoredFoldersIds.count() > 1)
        _monitoredFoldersIds.clear();

    context->client()->monitor(_monitoredFoldersIds);

    messageListCompleted(context);
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
    
    ImapSynchronizeBaseStrategy::handleLogin(context);
}

void ImapRetrieveMessageListStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    foreach (const QMailFolderId &folderId, _updatedFolders) {
        QMailFolder folder(folderId);

        bool modified(false);
        if (_newMinMaxMap.contains(folderId)) {
            folder.setCustomField("qmf-min-serveruid", QString::number(_newMinMaxMap[folderId].minimum()));
            folder.setCustomField("qmf-max-serveruid", QString::number(_newMinMaxMap[folderId].maximum()));
            modified = true;
        }

        if (folder.serverUndiscoveredCount() != 0) {
            folder.setServerUndiscoveredCount(0);
            modified = true;
        }

        if (modified && !QMailStore::instance()->updateFolder(&folder)) {
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
    QMailFolderId folderId(context->mailbox().id);
    QMailFolder folder(folderId);

    bool ok; // toUint returns 0 on error, which is an invalid IMAP uid
    int clientMin(folder.customField("qmf-min-serveruid").toUInt(&ok));
    int clientMax(folder.customField("qmf-max-serveruid").toUInt(&ok));

    // Use an optimization/simplification because client region should be contiguous
    IntegerRegion clientRegion;
    if ((clientMin != 0) && (clientMax != 0))
        clientRegion = IntegerRegion(clientMin, clientMax);
    IntegerRegion serverRegion;
    foreach(const QString &uid, context->mailbox().uidList) {
        bool ok;
        uint number = stripFolderPrefix(uid).toUInt(&ok);
        if (ok)
            serverRegion.add(number);
    }
    
    if (!_fillingGap) {
        if (!clientRegion.isEmpty() && !serverRegion.isEmpty()) {
            uint newestClient(clientRegion.maximum());
            uint oldestServer(serverRegion.minimum());
            uint newestServer(serverRegion.maximum());
            if (newestClient + 1 < oldestServer) {
                // There's a gap
                _fillingGap = true;
                context->protocol().sendUidSearch(MFlag_All, QString("UID %1:%2").arg(newestClient + 1).arg(newestServer));
                return;
            }
        }
    }
    
    // TODO: Why is all this code not in processUidSearchResults?
    _updatedFolders.append(folderId);

    IntegerRegion difference(serverRegion.subtract(clientRegion));
    if (difference.cardinality()) {
        _retrieveUids.append(qMakePair(folderId, difference.toStringList()));
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
            _newMinMaxMap.insert(folderId, IntegerRegion(newClientMin, newClientMax));
    }

    processUidSearchResults(context);
}

void ImapRetrieveMessageListStrategy::folderListFolderAction(ImapStrategyContextBase *context)
{
    // The current mailbox is now selected
    QMailFolderId folderId(context->mailbox().id);

    // Could get flag changes mod sequences when CONDSTORE is available
    
    bool comparable(true);
    int uidNext(-1);
    int exists(-1);
    int lastUidNext(-1);
    int lastExists(-1);
    if (_lastUidNextMap.contains(folderId))
        lastUidNext = _lastUidNextMap.value(folderId);
    if (_lastExistsMap.contains(folderId))
        lastExists = _lastExistsMap.value(folderId);
    
    // Cache uidnext and exists
    if (context->mailbox().isSelected()) {
        uidNext = context->mailbox().uidNext;
        exists = context->mailbox().exists;
        _lastUidNextMap.insert(folderId, uidNext);
        _lastExistsMap.insert(folderId, exists);
    }
    if ((uidNext < 0) || (exists < 0) || (lastUidNext < 0) || (lastExists < 0)) {
        comparable = false;
    }
    if (_minimum == 1) {
        // The don't select an already selected folder optimization is causing a problem,
        // we aren't picking up changes to uidNext or exists.
        // Work around this for push email at least for now.
        comparable = false;
    }
    
    // We're searching mailboxes
    int start = context->mailbox().exists - _minimum + 1;
    if (start < 1)
        start = 1;
    if ((context->mailbox().exists <= 0) || (_minimum <= 0)) {
        // No messages, so no need to perform search
        processUidSearchResults(context);
    } else if (!comparable || (lastExists != exists) || (lastUidNext != uidNext)) {
        context->protocol().sendUidSearch(MFlag_All, QString("%1:*").arg(start));
    } else {
        // No messages have been appended or expunged since we last retrieved messages
        // should be safe to incrementally retrieve
        QMailMessageKey folderKey(context->client()->messagesKey(folderId));
        QMailMessageKey folderTrashKey(context->client()->trashKey(folderId));
        uint onClient(QMailStore::instance()->countMessages(folderKey | folderTrashKey));
        if (onClient >= _minimum) {
            // We already have _minimum mails
            processUidSearchResults(context);
            return;
        }
        int end = context->mailbox().exists - onClient;
        if (end >= start) {
            context->protocol().sendUidSearch(MFlag_All, QString("%1:%2").arg(start).arg(end));
        } else {
            // We already have all the mails
            processUidSearchResults(context);
        }
    }
}

void ImapRetrieveMessageListStrategy::processUidSearchResults(ImapStrategyContextBase *context)
{
    processNextFolder(context);
}


/* A strategy to copy selected messages.
*/
// Note: the copy strategy is:
// 1. Select the destination folder, and find UIDNEXT
// 2. Copy each specified message to the destination
// 3. Select the destination folder
// 4. Search for messages >= previous UIDNEXT
// 5. Retrieve metadata only for found messages

// NB: the above doesn't work with Cyrus 2.2; instead, just use the \Recent flag to identify copies...

void ImapCopyMessagesStrategy::setDestination(const QMailFolderId& id)
{
    _destination = QMailFolder(id);
}

void ImapCopyMessagesStrategy::newConnection(ImapStrategyContextBase *context)
{
    _sourceUid.clear();
    _sourceUids.clear();
    _sourceIndex = 0;

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
        
        case IMAP_UIDSearch:
        {
            handleUidSearch(context);
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
    // We need to select the destination folder to discover it's UIDNEXT value
    if (_destination.id() == context->mailbox().id) {
        // We already have the appropriate mailbox selected
        handleSelect(context);
    } else {
        context->protocol().sendSelect(_destination);
    }
}

void ImapCopyMessagesStrategy::handleSelect(ImapStrategyContextBase *context)
{
    if (_transferState == Init) {
        //_uidNext = context->mailbox().uidNext;

        // Begin copying messages
        messageListMessageAction(context);
    } else if (_transferState == Search) {
        // We have selected the destination folder - find the newly added messages
        //context->protocol().sendUidSearch(MFlag_Recent, QString("UID %1:*").arg(ImapProtocol::uid(_uidNext)));
        context->protocol().sendUidSearch(MFlag_Recent);
    } else {
        ImapFetchSelectedMessagesStrategy::handleSelect(context);
    }
}

void ImapCopyMessagesStrategy::handleUidCopy(ImapStrategyContextBase *context)
{
    messageListMessageAction(context);
}

void ImapCopyMessagesStrategy::handleUidSearch(ImapStrategyContextBase *context)
{
    _createdUids = context->mailbox().uidList;
    fetchNextCopy(context);
}

void ImapCopyMessagesStrategy::handleUidFetch(ImapStrategyContextBase *context)
{
    fetchNextCopy(context);
}

void ImapCopyMessagesStrategy::messageListMessageAction(ImapStrategyContextBase *context)
{
    if (selectNextMessageSequence(context, 1)) {
        QStringList uids;
        foreach (const QString &msgUid, _retrieveUid.split("\n")) {
            _sourceUids.append(msgUid);
            uids.append(ImapProtocol::uid(msgUid));
        }

        context->updateStatus( QObject::tr("Copying %1 / %2").arg(_messageCount + 1).arg(_listSize) );
        _messageCount += uids.count();

        context->protocol().sendUidCopy( IntegerRegion(uids).toString(), _destination );
    }
}

void ImapCopyMessagesStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    if (_transferState == Search) {
        ImapFetchSelectedMessagesStrategy::messageListCompleted(context);
    } else {
        // We have copied all the messages - now we need to retrieve the copies
        _transferState = Search;
        context->protocol().sendSelect(_destination);
    }
}

void ImapCopyMessagesStrategy::messageCopied(ImapStrategyContextBase *context, const QString &copiedUid, const QString &createdUid)
{
    if (!createdUid.isEmpty()) {
        _sourceUid[createdUid] = copiedUid;
        _sourceUids.removeAll(copiedUid);
    }

    ImapFetchSelectedMessagesStrategy::messageCopied(context, copiedUid, createdUid);
}

void ImapCopyMessagesStrategy::messageFetched(ImapStrategyContextBase *context, QMailMessage &message)
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
        const QMailMessage source(sourceUid, context->config().id());
        if (source.id().isValid()) {
            updateCopiedMessage(context, message, source);
        } else {
            qWarning() << "Unable to update message from UID:" << sourceUid << "to copy:" << message.serverUid();
        }
    }

    ImapFetchSelectedMessagesStrategy::messageFetched(context, message);
}

void ImapCopyMessagesStrategy::updateCopiedMessage(ImapStrategyContextBase *, QMailMessage &message, const QMailMessage &source)
{ 
    if ((source.status() & QMailMessage::New) == 0) {
        message.setStatus(QMailMessage::New, false);
    }
    if (source.status() & QMailMessage::Read) {
        message.setStatus(QMailMessage::Read, true);
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


/* A strategy to move selected messages.
*/
// Note: the move strategy is:
// 1. Select the destination folder, and find UIDNEXT
// 2. Copy each specified message to the destination, recording the local ID
// 3. Mark each specified message as deleted
// 4. Close each modified folder to expunge the marked messages
// 5. Update the status for each modified folder
// 6. Select the destination folder
// 7. Search for messages >= previous UIDNEXT
// 8. Retrieve metadata only for found messages
// 9. When the metadata for a copy is fetched, move the local body of the source message into the copy

void ImapMoveMessagesStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_UIDStore:
        {
            handleUidStore(context);
            break;
        }
        
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
            ImapCopyMessagesStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapMoveMessagesStrategy::handleUidCopy(ImapStrategyContextBase *context)
{
    // Mark the copied message(s) as deleted
    QStringList uids;
    foreach (const QString &msgUid, _retrieveUid.split("\n"))
        uids.append(ImapProtocol::uid(msgUid));

    context->protocol().sendUidStore(MFlag_Deleted, IntegerRegion(uids).toString());
}

void ImapMoveMessagesStrategy::handleUidStore(ImapStrategyContextBase *context)
{
    _lastMailbox = _currentMailbox;
    messageListMessageAction(context);
}

void ImapMoveMessagesStrategy::handleClose(ImapStrategyContextBase *context)
{
    context->protocol().sendExamine(_lastMailbox);
    _lastMailbox = QMailFolder();
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
    if (selectNextMessageSequence(context, 1)) {
        QStringList uids;
        foreach (const QString &msgUid, _retrieveUid.split("\n")) {
            _sourceUids.append(msgUid);
            uids.append(ImapProtocol::uid(msgUid));
        }

        context->updateStatus( QObject::tr("Moving %1 / %2").arg(_messageCount + 1).arg(_listSize) );
        _messageCount += uids.count();

        context->protocol().sendUidCopy( IntegerRegion(uids).toString(), _destination );
    }
}

void ImapMoveMessagesStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    if (_transferState == Search) {
        // We don't need to keep the source messages any longer
        QStringList allSourceUids(_sourceUids + _sourceUid.values());
        QMailMessageKey uidKey(QMailMessageKey::serverUid(allSourceUids));
        QMailMessageKey accountKey(QMailMessageKey::parentAccountId(context->config().id()));

        if (!QMailStore::instance()->removeMessages(accountKey & uidKey, QMailStore::NoRemovalRecord)) {
            qWarning() << "Unable to remove message for account:" << context->config().id() << "UIDs:" << allSourceUids;
        }
    }

    ImapCopyMessagesStrategy::messageListCompleted(context);
}

static void transferPartBodies(QMailMessagePartContainer &destination, const QMailMessagePartContainer &source)
{
    Q_ASSERT(destination.partCount() == source.partCount());

    if (source.hasBody()) {
        destination.setBody(source.body());
    } else if (source.partCount() > 0) {
        for (uint i = 0; i < source.partCount(); ++i) {
            const QMailMessagePart &sourcePart = source.partAt(i);
            QMailMessagePart &destinationPart = destination.partAt(i);

            transferPartBodies(destinationPart, sourcePart);
        }
    }
}

void ImapMoveMessagesStrategy::updateCopiedMessage(ImapStrategyContextBase *context, QMailMessage &message, const QMailMessage &source)
{ 
    ImapCopyMessagesStrategy::updateCopiedMessage(context, message, source);

    transferPartBodies(message, source);

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
}

/* A strategy to delete selected messages.
*/
void ImapDeleteMessagesStrategy::setLocalMessageRemoval(bool removal)
{
    _removal = removal;
}

void ImapDeleteMessagesStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_UIDStore:
        {
            handleUidStore(context);
            break;
        }
        
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
            ImapFetchSelectedMessagesStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapDeleteMessagesStrategy::handleUidStore(ImapStrategyContextBase *context)
{
    // Add the stored UIDs to our list
    _storedList += _retrieveUid.split("\n");
    _lastMailbox = _currentMailbox;

    messageListMessageAction(context);
}

void ImapDeleteMessagesStrategy::handleClose(ImapStrategyContextBase *context)
{
    if (_removal) {
        // All messages in the stored list are now expunged - delete the local copies
        QMailMessageKey accountKey(QMailMessageKey::parentAccountId(context->config().id()));
        QMailMessageKey uidKey(QMailMessageKey::serverUid(_storedList));

        if (!QMailStore::instance()->removeMessages(accountKey & uidKey, QMailStore::NoRemovalRecord)) {
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
    ImapFetchSelectedMessagesStrategy::messageListFolderAction(context);
}

void ImapDeleteMessagesStrategy::messageListMessageAction(ImapStrategyContextBase *context)
{
    if (selectNextMessageSequence(context, 1000)) {
        QStringList uids;
        foreach (const QString &msgUid, _retrieveUid.split("\n"))
            uids.append(ImapProtocol::uid(msgUid));

        context->protocol().sendUidStore(MFlag_Deleted, IntegerRegion(uids).toString());
    }
}

void ImapDeleteMessagesStrategy::messageListFolderAction(ImapStrategyContextBase *context)
{
    // If we're already in a mailbox, we need to close it to expunge the messages
    if ((context->mailbox().isSelected()) && (_lastMailbox.id().isValid())) {
        context->protocol().sendClose();
    } else {
        // Select the next folder, and clear the stored list
        _storedList.clear();
        ImapFetchSelectedMessagesStrategy::messageListFolderAction(context);
    }
}

void ImapDeleteMessagesStrategy::messageListCompleted(ImapStrategyContextBase *context)
{
    // If we're already in a mailbox, we need to close it to expunge the messages
    if ((context->mailbox().isSelected()) && (_lastMailbox.id().isValid())) {
        context->protocol().sendClose();
    } else {
        ImapFetchSelectedMessagesStrategy::messageListCompleted(context);
    }
}


ImapStrategyContext::ImapStrategyContext(ImapClient *client)
  : ImapStrategyContextBase(client), 
    _strategy(0) 
{
}
