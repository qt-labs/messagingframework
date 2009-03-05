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

// Enable this to use some simple command pipelining
//#define Q_PIPELINE_COMMANDS

static const int MetaDataFetchFlags = F_Uid | F_Date | F_Rfc822_Size | F_Rfc822_Header | F_BodyStructure;
static const int ContentFetchFlags = F_Uid | F_Rfc822_Size | F_Rfc822;

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
   or message parts to operate on, and an abstract interface messageAction() 
   for operating on messages.
   
   Also implements logic to determine which messages or message part to operate 
   on next.
*/
void ImapMessageListStrategy::setSelectedMails(const QMailMessageIdList& ids) 
{
    _selectionMap.clear();
    if (ids.count() == 0)
        return;

    const QMailFolderId trashFolderId(QMailFolder::TrashFolder);

    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentFolderId | QMailMessageKey::PreviousParentFolderId | QMailMessageKey::ServerUid);
    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(ids), props)) {
        // TODO - order messages within each folder by size ascending, or most recent first?
        QMailFolderId parentFolderId(metaData.parentFolderId() == trashFolderId ? metaData.previousParentFolderId() : metaData.parentFolderId());
        _selectionMap[parentFolderId].insert(metaData.serverUid(), SectionProperties(metaData.id()));
    }

    _folderItr = _selectionMap.begin();
    _selectionItr = _folderItr.value().begin();
}

void ImapMessageListStrategy::setSelectedSection(const QMailMessagePart::Location &location)
{
    _selectionMap.clear();

    QMailMessageMetaData metaData(location.containingMessageId());
    if (metaData.id().isValid()) {
        SectionProperties sectionProperties(metaData.id(), location);
        _selectionMap[metaData.parentFolderId()].insert(metaData.serverUid(), sectionProperties);
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
            qWarning("Unhandled IMAP response!");
            break;
        }
    }
}

void ImapMessageListStrategy::handleLogin(ImapStrategyContextBase *context)
{
    messageAction(context);
}

void ImapMessageListStrategy::handleSelect(ImapStrategyContextBase *context)
{
#ifndef Q_PIPELINE_COMMANDS
    // We're completing a message or section
    messageAction(context);
#else
    Q_UNUSED(context);
#endif
}

bool ImapMessageListStrategy::computeStartEndPartRange(ImapStrategyContextBase *context)
{
    if (!_retrieveUid.isEmpty()) {
        QMailMessage mail(_retrieveUid, context->config().id());
        if (mail.id().isValid()) {
            QString sectionStr;
            if (_msgSection.isValid(false)) {
                sectionStr = _msgSection.toString(false);
            }
            // Update the relevant part
            QString tempDir = QMail::tempPath() + QDir::separator();
            QFile partFile(tempDir + "mail-" + _retrieveUid + "-part-" + sectionStr);
            _sectionStart = 0;
            if (partFile.exists()) {
                _sectionStart = partFile.size();
            }
            if (_sectionEnd < _sectionStart) {
                qWarning() << "Cleaning up stale partFile" << partFile.fileName();
                partFile.remove();
                _sectionStart = 0;
            }
            return true;
        }
    }
    return false;
}

bool ImapMessageListStrategy::selectNextMessageSequence(ImapStrategyContextBase *context, int maximum)
{
    QMailFolderId mailboxId;
    QStringList uidList;
    QMailMessagePart::Location location;
    int minimum = SectionProperties::All;

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
        folderAction(context);
        return false;
    }

    _transferState = Complete;
    mailboxId = _folderItr.key();
    if (mailboxId != _currentMailbox.id()) {
        _currentMailbox = QMailFolder(mailboxId);
        folderAction(context);
        return false;
    }

    while ((_selectionItr != selectionEnd) && (uidList.count() < maximum)) {
        uidList.append(_selectionItr.key());
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

void ImapMessageListStrategy::folderAction(ImapStrategyContextBase *context)
{
    if (_currentMailbox.id().isValid()) {
        if (_currentMailbox.id() == context->mailbox().id) {
            // We already have the appropriate mailbox selected
            handleSelect(context);
        } else {
            context->protocol().sendSelect(_currentMailbox);
        }
#ifdef Q_PIPELINE_COMMANDS
        messageAction(context);
#endif
    } else {
        completedAction(context);
    }
}

void ImapMessageListStrategy::completedAction(ImapStrategyContextBase *context)
{
    context->operationCompleted();
}


/* A strategy which provides an interface for defining a set of messages
   or message parts to retrieve, and logic to retrieve those messages
   or message parts and emit progress notifications.
*/
void ImapFetchSelectedMessagesStrategy::setOperation(QMailRetrievalAction::RetrievalSpecification spec)
{
    if (spec == QMailRetrievalAction::MetaData) {
        _headerLimit = UINT_MAX;
    } else {
        _headerLimit = 0;
    }
}

void ImapFetchSelectedMessagesStrategy::setSelectedMails(const QMailMessageIdList& ids) 
{
    // We shouldn't have anything left in our retrieval list...
    if (!_retrievalSize.isEmpty()) {
        foreach (const QString& uid, _retrievalSize.keys())
            qMailLog(IMAP) << "Message" << uid << "still in retrieve map...";

        _retrievalSize.clear();
    }

    _totalRetrievalSize = 0;
    _selectionMap.clear();
    _listSize = ids.count();
    if (_listSize == 0)
        return;

    const QMailFolderId trashFolderId(QMailFolder::TrashFolder);

    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentFolderId | QMailMessageKey::PreviousParentFolderId | QMailMessageKey::ServerUid | QMailMessageKey::Size);
    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(ids), props)) {
        // TODO - order messages within each folder by size ascending, or most recent first?
        QMailFolderId parentFolderId(metaData.parentFolderId() == trashFolderId ? metaData.previousParentFolderId() : metaData.parentFolderId());
        _selectionMap[parentFolderId].insert(metaData.serverUid(), SectionProperties(metaData.id()));

        uint size = metaData.indicativeSize();
        uint bytes = metaData.size();

        _retrievalSize.insert(metaData.serverUid(), qMakePair(qMakePair(size, bytes), 0u));
        _totalRetrievalSize += size;
    }

    _folderItr = _selectionMap.begin();
    _selectionItr = _folderItr.value().begin();

    // Report the total size we will retrieve
    _progressRetrievalSize = 0;
}

void ImapFetchSelectedMessagesStrategy::setSelectedSection(const QMailMessagePart::Location &location, int minimum)
{
    // We shouldn't have anything left in our retrieval list...
    if (!_retrievalSize.isEmpty()) {
        foreach (const QString& uid, _retrievalSize.keys())
            qMailLog(IMAP) << "Message" << uid << "still in retrieve map...";

        _retrievalSize.clear();
    }

    _totalRetrievalSize = 0;
    _selectionMap.clear();

    QMailMessageMetaData metaData(location.containingMessageId());
    if (metaData.id().isValid()) {
        SectionProperties sectionProperties(metaData.id(), location, minimum);
        _selectionMap[metaData.parentFolderId()].insert(metaData.serverUid(), sectionProperties);

        uint size = metaData.indicativeSize();
        uint bytes = metaData.size();
        if (minimum > 0)
            size = bytes = minimum;

        _retrievalSize.insert(metaData.serverUid(), qMakePair(qMakePair(size, bytes), 0u));
        _totalRetrievalSize += size;
    }

    _folderItr = _selectionMap.begin();
    _selectionItr = _folderItr.value().begin();

    _listSize = 1;
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

    messageAction(context);
}

void ImapFetchSelectedMessagesStrategy::handleUidFetch(ImapStrategyContextBase *context)
{
    messageAction(context);
}

void ImapFetchSelectedMessagesStrategy::messageAction(ImapStrategyContextBase *context)
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


/* A strategy to retrieve a list of all folders a.ka. mailboxes in an account.
   */
void ImapFolderListStrategy::setBase(const QMailFolderId &folderId)
{
    _baseId = folderId;
}

void ImapFolderListStrategy::setDescending(bool descending)
{
    _descending = descending;
}

void ImapFolderListStrategy::newConnection(ImapStrategyContextBase *context)
{
    _folderStatus.clear();
    _mailboxList.clear();

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
        
        default:
        {
            ImapFetchSelectedMessagesStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapFolderListStrategy::handleLogin(ImapStrategyContextBase *context)
{
    context->updateStatus( QObject::tr("Retrieving folders") );
    _mailboxPaths.clear();

    _transferState = List;
    ImapConfiguration imapCfg(context->config());

    QMailFolder folder;
    if (_baseId.isValid()) {
        folder = QMailFolder(_baseId);
    } else {
        QMailFolderId folderId = context->client()->mailboxId(imapCfg.baseFolder());
        if (folderId.isValid()) {
            folder = QMailFolder(folderId);
        }
    }

    context->protocol().sendList(folder, "%");
}

void ImapFolderListStrategy::handleList(ImapStrategyContextBase *context)
{
    // Search for folders in the discovered mailboxes
    while (!_mailboxIds.isEmpty()) {
        QMailFolderId folderId(_mailboxIds.takeFirst());
        
        FolderStatus folderState = _folderStatus[folderId];
        if (!(folderState & NoInferiors) && !(folderState & HasNoChildren)) {
            context->protocol().sendList(QMailFolder(folderId), "%");
            return;
        }
    }

    _mailboxList = context->client()->mailboxIds();

    listCompleted(context);
}

void ImapFolderListStrategy::listCompleted(ImapStrategyContextBase *context)
{
    removeDeletedMailboxes(context);

    // We have retrieved all the folders
    completedAction(context);
}

void ImapFolderListStrategy::mailboxListed(ImapStrategyContextBase *context, QMailFolder &folder, const QString &flags)
{
    ImapStrategy::mailboxListed(context, folder, flags);

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
    _mailboxPaths.append(folder.path());

    if (_descending) {
        _mailboxIds.append(folder.id());
    }
}

void ImapFolderListStrategy::removeDeletedMailboxes(ImapStrategyContextBase *context)
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

/* An abstract strategy. To be used as a base class for strategies
   that iteratively search all mailboxes in an account and/or
   iterate over mailboxes Previewing and Completing discovered mails.
*/
QStringList ImapSynchronizeBaseStrategy::inFirstAndSecond(const QStringList &first, const QStringList &second)
{
    QStringList result;

    foreach (const QString &value, first)
        if (second.contains(value))
            result.append(value);

    return result;
}

QStringList ImapSynchronizeBaseStrategy::inFirstButNotSecond(const QStringList &first, const QStringList &second)
{
    QStringList result;

    foreach (const QString &value, first)
        if (!second.contains(value))
            result.append(value);

    return result;
}

void ImapSynchronizeBaseStrategy::newConnection(ImapStrategyContextBase *context)
{
    _retrieveUids.clear();
    ImapFolderListStrategy::newConnection(context);
}

void ImapSynchronizeBaseStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
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

void ImapSynchronizeBaseStrategy::handleLogin(ImapStrategyContextBase *context)
{
    _completionList.clear();

    ImapFolderListStrategy::handleLogin(context);
}

void ImapSynchronizeBaseStrategy::listCompleted(ImapStrategyContextBase *context)
{
    if (!selectNextMailbox(context)) {
        // Could be no mailbox has been selected to be stored locally
        completedAction(context);
    }
}

bool ImapSynchronizeBaseStrategy::selectNextMailbox(ImapStrategyContextBase *context)
{
    _newUids = QStringList();

    if (nextMailbox(context)) {
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
    } else {
        return false;
    }
}

bool ImapSynchronizeBaseStrategy::nextMailbox(ImapStrategyContextBase *)
{
    while (!_mailboxList.isEmpty()) {
        _currentMailbox = QMailFolder(_mailboxList.takeFirst());
        if (_currentMailbox.status() & QMailFolder::SynchronizationEnabled)
            return true;
    }

    // We have exhausted the list of mailboxes
    if ((_transferState == Preview) && !_retrieveUids.isEmpty()) {
        // In fetch mode, return the mailboxes where retrievable messages are located
        QPair<QMailFolderId, QStringList> next = _retrieveUids.takeFirst();
        _currentMailbox = QMailFolder(next.first);
        _newUids = next.second;
        return true;
    }

    _currentMailbox = QMailFolder();
    return false;
}

void ImapSynchronizeBaseStrategy::handleSelect(ImapStrategyContextBase *context)
{
    // We have selected the current mailbox
    if (_transferState == Preview) {
        // We're retrieving message metadata
        fetchNextMailPreview(context);
    } else if (_transferState == Complete) {
#ifndef Q_PIPELINE_COMMANDS
        // We're completing a message or section
        messageAction(context);
#endif
    }
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
    } else if (!selectNextMailbox(context)) {
        if ((_transferState == Preview) || (_retrieveUids.isEmpty())) {
            if (!_completionList.isEmpty()) {
                // Fetch the messages that need completion
                setSelectedMails(_completionList);
                messageAction(context);
            } else {
                context->operationCompleted();
            }
        } else {
            // We now have a list of all messages to be retrieved for each mailbox
            _total = 0;
            QList<QPair<QMailFolderId, QStringList> >::const_iterator it = _retrieveUids.begin(), end = _retrieveUids.end();
            for ( ; it != end; ++it)
                _total += it->second.count();

            context->updateStatus(QObject::tr("Previewing", "Previewing <number of messages>") + QChar(' ') + QString::number(_total));

            _progress = 0;
            context->progressChanged(_progress, _total);

            _transferState = Preview;
            selectNextMailbox(context);
        }
    }
}

void ImapSynchronizeBaseStrategy::messageFetched(ImapStrategyContextBase *context, QMailMessage &message)
{ 
    ImapFolderListStrategy::messageFetched(context, message);

    if (_transferState == Preview) {
        context->progressChanged(_progress++, _total);

        if (((message.status() & QMailMessage::Downloaded) == 0) && (message.size() < _headerLimit)) {
            _completionList.append(message.id());
        }
    }
}

void ImapSynchronizeBaseStrategy::handleUidFetch(ImapStrategyContextBase *context)
{
    if (_transferState == Preview) {    //getting headers
        fetchNextMailPreview(context);
    } else if (_transferState == Complete) {    //getting complete messages
        messageAction(context);
    }
}

/* A strategy to provide full account synchronization. 
   
   That is to export and import changes, to retrieve message previews for all 
   known messages in an account, and to complete messages where appropriate.
*/
ImapSynchronizeAllStrategy::ImapSynchronizeAllStrategy()
{
    _options = (Options)(RetrieveMail | ImportChanges | ExportChanges);
}

void ImapSynchronizeAllStrategy::setOptions(Options options)
{
    _options = options;
}

void ImapSynchronizeAllStrategy::transition(ImapStrategyContextBase *context, ImapCommand command, OperationStatus status)
{
    switch( command ) {
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
            ImapSynchronizeBaseStrategy::transition(context, command, status);
            break;
        }
    }
}

void ImapSynchronizeAllStrategy::listCompleted(ImapStrategyContextBase *context)
{
    removeDeletedMailboxes(context);

    if (!selectNextMailbox(context)) {
        // Could be no mailbox has been selected to be stored locally
        completedAction(context);
    }
}

bool ImapSynchronizeAllStrategy::selectNextMailbox(ImapStrategyContextBase *context)
{
    _seenUids = QStringList();
    _unseenUids = QStringList();
    _readUids = QStringList();
    _removedUids = QStringList();
    _expungeRequired = false;
    
    _searchState = Seen;
    return ImapSynchronizeBaseStrategy::selectNextMailbox(context);
}

void ImapSynchronizeAllStrategy::handleSelect(ImapStrategyContextBase *context)
{
    // We have selected the current mailbox
    if (_transferState == List) {
        // We're searching mailboxes
        if (context->mailbox().exists > 0) {
            // Start by looking for previously-seen and unseen messages
            context->protocol().sendUidSearch(MFlag_Seen);
#ifdef Q_PIPELINE_COMMANDS
            context->protocol().sendUidSearch(MFlag_Unseen);
#endif
        } else {
            // No messages, so no need to perform search
            processUidSearchResults(context);
        }
    } else {
        ImapSynchronizeBaseStrategy::handleSelect(context);
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
#ifndef Q_PIPELINE_COMMANDS
        context->protocol().sendUidSearch(MFlag_Unseen);
#endif
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

void ImapSynchronizeAllStrategy::processUidSearchResults(ImapStrategyContextBase *context)
{
    QMailFolderId boxId = _currentMailbox.id();

    if ((_currentMailbox.status() & QMailFolder::SynchronizationEnabled) &&
        !(_currentMailbox.status() & QMailFolder::Synchronized)) {
        // We have just synchronized this folder
        QMailFolder folder(boxId);
        folder.setStatus(QMailFolder::Synchronized, true);
        if (!QMailStore::instance()->updateFolder(&folder)) {
            qWarning() << "Unable to update folder for account:" << context->config().id();
        }
    }

    // Compare the server message list with our message list
    QStringList reportedUids = _seenUids + _unseenUids;

    QStringList readElsewhereUids = context->client()->serverUids(boxId, QMailMessage::ReadElsewhere);
    QStringList unreadElsewhereUids = context->client()->serverUids(boxId, QMailMessage::ReadElsewhere, false);
    QStringList deletedUids = context->client()->deletedMessages(boxId);

    QStringList storedUids = readElsewhereUids + unreadElsewhereUids + deletedUids;

    // New messages reported by the server that we don't yet have
    if (_options & RetrieveMail) {
        QStringList newUids = inFirstButNotSecond(reportedUids, storedUids);
        if (!newUids.isEmpty()) {
            // Add this folder to the list to retrieve from later
            _retrieveUids.append(qMakePair(boxId, newUids));
        }
    }

    if (_searchState == Inconclusive) {
        // Don't mark or delete any messages without a correct server listing
        searchInconclusive(context);
    } else {
        // Only delete messages the server still has
        _removedUids = inFirstAndSecond(deletedUids, reportedUids);
        _expungeRequired = !_removedUids.isEmpty();

        // Messages marked read locally that the server reports are unseen
        _readUids = inFirstAndSecond(context->client()->serverUids(boxId, QMailMessage::Read), _unseenUids);

        if (!(_options & ImportChanges)) {
            handleUidStore(context);
            return;
        }

        // Mark as deleted any messages that the server does not report
        QStringList nonexistentUids = inFirstButNotSecond(storedUids, reportedUids);

        QMailMessageKey accountKey(QMailMessageKey::parentAccountId(context->config().id()));
        QMailMessageKey uidKey(QMailMessageKey::serverUid(nonexistentUids));
        if (!QMailStore::instance()->updateMessagesMetaData(accountKey & uidKey, QMailMessage::Removed, true)) {
            qWarning() << "Unable to update message metadata for account:" << context->config().id();
        }

        foreach (const QString &uid, nonexistentUids)  {
            // We might have a deletion record for this UID
            if (!QMailStore::instance()->purgeMessageRemovalRecords(context->config().id(), QStringList() << uid)) {
                qWarning() << "Unable to purge message records for account:" << context->config().id();
            }
            context->completedMessageAction(uid);
        }

        // Update any messages that are reported read-elsewhere, that we didn't previously know about
        foreach (const QString &uid, inFirstAndSecond(_seenUids, unreadElsewhereUids)) {
            // We know of this message, but we have it marked as unread
            QMailMessageMetaData metaData(uid, context->config().id());
            metaData.setStatus(QMailMessage::ReadElsewhere, true);
            if (!QMailStore::instance()->updateMessage(&metaData)) {
                qWarning() << "Unable to update message for account:" << context->config().id();
            }
        }

        // Mark any messages that we have read that the server thinks are unread
        handleUidStore(context);
    }
}

void ImapSynchronizeAllStrategy::searchInconclusive(ImapStrategyContextBase *context)
{
    fetchNextMailPreview(context);
}

void ImapSynchronizeAllStrategy::handleUidStore(ImapStrategyContextBase *context)
{
    if (!(_options & ExportChanges)) {
        fetchNextMailPreview(context);
        return;
    }
    if (!setNextSeen(context))
        if (!setNextDeleted(context))
            fetchNextMailPreview(context);
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

void ImapSynchronizeAllStrategy::handleExpunge(ImapStrategyContextBase *context)
{
    fetchNextMailPreview(context);
}

void ImapSynchronizeAllStrategy::fetchNextMailPreview(ImapStrategyContextBase *context)
{
    if (!(_options & RetrieveMail)) {
        context->operationCompleted();
        return;
    }
    
    ImapSynchronizeBaseStrategy::fetchNextMailPreview(context);
}

/* A strategy to retrieve all messages from an account.
   
   That is to retrieve message previews for all known messages 
   in an account, and to complete messages where appropriate.
*/
ImapRetrieveAllStrategy::ImapRetrieveAllStrategy()
{
    setOptions((Options)(RetrieveMail | ImportChanges));
}

/* A strategy to exports changes made on the client to
   the server.
   
   That is to export flag changes (unseen -> seen) and
   deletes.
*/
ImapExportUpdatesStrategy::ImapExportUpdatesStrategy()
{
    setOptions((Options)(ExportChanges));
}


static QStringList stripFolderPrefix(const QStringList &list)
{
    QStringList result;
    foreach(const QString &uid, list) {
        int index = 0;
        if ((index = uid.lastIndexOf('|')) != -1)
            result.append(uid.mid(index + 1));
        else
            result.append(uid);
    }
    return result;
}

/* A strategy to update message flags for a list of messages.
   
   That is to detect changes to flags (unseen->seen) 
   and to detect deleted mails.
*/
void ImapUpdateMessagesFlagsStrategy::setSelectedMails(const QMailMessageIdList &messageIds)
{
    _messageIds = messageIds;
}

void ImapUpdateMessagesFlagsStrategy::handleLogin(ImapStrategyContextBase *context)
{
    _serverUids.clear();
    _folderId = QMailFolderId();
    _transferState = List;
    _searchState = Seen;
    if (!selectNextMailbox(context))
        context->operationCompleted();
}

bool ImapUpdateMessagesFlagsStrategy::selectNextMailbox(ImapStrategyContextBase *context)
{
    QMailFolderId folderId;
    QMutableListIterator<QMailMessageId> it(_messageIds);
    _serverUids.clear();
    while (it.hasNext()) {
        QMailMessageId id(it.next());
        if (!id.isValid()) {
            _messageIds.removeAll(id);
            continue;
        }
        QMailMessageMetaData metaData(id);
        if (!metaData.parentFolderId().isValid()) {
            _messageIds.removeAll(id);
            continue;
        }
        if (!folderId.isValid())
            folderId = metaData.parentFolderId();
        if (metaData.parentFolderId() == folderId) {
            _serverUids.append(metaData.serverUid());
            _messageIds.removeAll(id);
        }
    }
    if (_serverUids.isEmpty() || !folderId.isValid())
        return false;

    _folderId = folderId;
    context->protocol().sendSelect(QMailFolder(folderId));
    return true;
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
#ifdef Q_PIPELINE_COMMANDS
            context->protocol().sendUidSearch(MFlag_Unseen, "UID " + _filter);
#endif
        } else {
            // No messages, so no need to perform search
            processUidSearchResults(context);
        }
    } else {
        ImapSynchronizeBaseStrategy::handleSelect(context);
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
#ifndef Q_PIPELINE_COMMANDS
        context->protocol().sendUidSearch(MFlag_Unseen, "UID " + _filter);
#endif
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
        context->operationCompleted();
    }
}

void ImapUpdateMessagesFlagsStrategy::processUidSearchResults(ImapStrategyContextBase *context)
{
    if (!_folderId.isValid()) {
        // Folder was removed while we were updating messages flags in it
        fetchNextMailPreview(context);
        return;
    }
    
    // Compare the server message list with our message list
    QStringList reportedUids = _seenUids + _unseenUids;

    QStringList readElsewhereUids = context->client()->serverUids(_folderId, QMailMessage::ReadElsewhere);
    QStringList unreadElsewhereUids = context->client()->serverUids(_folderId, QMailMessage::ReadElsewhere, false);
    QStringList deletedUids = context->client()->deletedMessages(_folderId);

    QStringList storedUids = readElsewhereUids + unreadElsewhereUids + deletedUids;

    // Code duplicated from ImapSynchronizeAllStrategy::processUidSearchResults below
    // Could make it a function of storedUids, reportedUids, nonexistentUids, _seenUids, unreadElsewhereUids
    
    // Mark as deleted any messages that the server does not report
    QStringList nonexistentUids = inFirstButNotSecond(storedUids, reportedUids);

    QMailMessageKey accountKey(QMailMessageKey::parentAccountId(context->config().id()));
    QMailMessageKey uidKey(QMailMessageKey::serverUid(nonexistentUids));
    QMailStore::instance()->updateMessagesMetaData(accountKey & uidKey, QMailMessage::Removed, true);

    foreach (const QString &uid, nonexistentUids)  {
        // We might have a deletion record for this UID
        QMailStore::instance()->purgeMessageRemovalRecords(context->config().id(), QStringList() << uid);
        context->completedMessageAction(uid);
    }

    // Update any messages that are reported read-elsewhere, that we didn't previously know about
    foreach (const QString &uid, inFirstAndSecond(_seenUids, unreadElsewhereUids)) {
        // We know of this message, but we have it marked as unread
        QMailMessageMetaData metaData(uid, context->config().id());
        metaData.setStatus(QMailMessage::ReadElsewhere, true);
        QMailStore::instance()->updateMessage(&metaData);
    }

    fetchNextMailPreview(context);
}


/* A strategy to ensure a given number of messages, for a given
   mailbox are on the client, retrieving messages if necessary.

   Retrieval order is defined by server uid.
*/
void ImapRetrieveMessageListStrategy::setFolder(const QMailFolderId &folderId, uint minimum)
{
    _folderId = folderId;
    _minimum = minimum;
}

void ImapRetrieveMessageListStrategy::handleLogin(ImapStrategyContextBase *context)
{
    context->updateStatus(QObject::tr("Scanning folder"));
    _transferState = List;
    _fillingGap = false;
    _completionList.clear();
    
    if (_folderId.isValid()) {
        if (_folderId == context->mailbox().id) {
            // We already have the appropriate mailbox selected
            handleSelect(context);
        } else {
            context->protocol().sendSelect(QMailFolder(_folderId));
        }
    } else {
        ImapSynchronizeBaseStrategy::handleLogin(context);
    }
}

void ImapRetrieveMessageListStrategy::listCompleted(ImapStrategyContextBase *context)
{
    removeDeletedMailboxes(context);

    if (!selectNextMailbox(context)) {
        // Could be no mailbox has been selected to be stored locally
        completedAction(context);
    }
}

void ImapRetrieveMessageListStrategy::handleSelect(ImapStrategyContextBase *context)
{
    // We have selected the current mailbox
    if (_transferState == List) {
        _folderId = context->mailbox().id;
        // Could get flag changes mod sequences when CONDSTORE is available
        
        bool comparable(true);
        int uidNext(-1);
        int exists(-1);
        int lastUidNext(-1);
        int lastExists(-1);
        if (_lastUidNextMap.contains(_folderId))
            lastUidNext = _lastUidNextMap.value(_folderId);
        if (_lastExistsMap.contains(_folderId))
            lastExists = _lastExistsMap.value(_folderId);
        
        // Cache uidnext and exists
        if (context->mailbox().isSelected()) {
            uidNext = context->mailbox().uidNext;
            exists = context->mailbox().exists;
            QMailFolder folder(context->mailbox().id);
            _lastUidNextMap.insert(_folderId, uidNext);
            _lastExistsMap.insert(_folderId, exists);
        }
        if ((uidNext < 0) || (exists < 0) || (lastUidNext < 0) || (lastExists < 0))
            comparable = false;
        
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
            // No messages have been appended or expunged since we last retrieve messages
            // should be safe to incrementally retrieve
            uint onClient(context->client()->serverUids(_folderId).count());
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
    } else {
        ImapSynchronizeBaseStrategy::handleSelect(context);
    }
}

void ImapRetrieveMessageListStrategy::handleUidSearch(ImapStrategyContextBase *context)
{
    IntegerRegion clientRegion(stripFolderPrefix(context->client()->serverUids(_folderId)));
    _serverRegion = IntegerRegion(stripFolderPrefix(context->mailbox().uidList));
    // clientList and serverList should be sorted numerically
    
    if (!_fillingGap) {
        bool ok1, ok2;
        QStringList clientList(clientRegion.toStringList());
        QStringList serverList(_serverRegion.toStringList());
        if (!clientList.isEmpty() && !serverList.isEmpty()) {
            uint newestClient(clientList.last().toUInt(&ok1));
            uint oldestServer(serverList.first().toUInt(&ok2));
            if (ok1 && ok2 && (newestClient < oldestServer)) {
                // There's a gap
                _fillingGap = true;
                context->protocol().sendUidSearch(MFlag_All, QString("UID %1:%2").arg(newestClient).arg(serverList.last()));
                return;
            }
        }
    } else {
        // TODO: IntegerRegion union
        foreach(const QString &uid, context->mailbox().uidList) {
            bool ok;
            uint number = uid.toUInt(&ok);
            if (ok)
                _serverRegion.add(number);
        }
    }
    
    IntegerRegion difference(_serverRegion.subtract(clientRegion));
    if (difference.cardinality()) {
        _retrieveUids.append(qMakePair(_folderId, difference.toStringList()));
    }

    processUidSearchResults(context);
}

void ImapRetrieveMessageListStrategy::processUidSearchResults(ImapStrategyContextBase *context)
{
    fetchNextMailPreview(context);
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
        messageAction(context);
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
    messageAction(context);
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

void ImapCopyMessagesStrategy::messageAction(ImapStrategyContextBase *context)
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

void ImapCopyMessagesStrategy::completedAction(ImapStrategyContextBase *context)
{
    if (_transferState == Search) {
        ImapFetchSelectedMessagesStrategy::completedAction(context);
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
        completedAction(context);
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
    messageAction(context);
}

void ImapMoveMessagesStrategy::handleClose(ImapStrategyContextBase *context)
{
    context->protocol().sendExamine(_lastMailbox);
    _lastMailbox = QMailFolder();
}

void ImapMoveMessagesStrategy::handleExamine(ImapStrategyContextBase *context)
{
    // Select the next folder
    ImapCopyMessagesStrategy::folderAction(context);
}

void ImapMoveMessagesStrategy::handleUidFetch(ImapStrategyContextBase *context)
{
    // See if we can move the body of the source message to this message
    fetchNextCopy(context);
}

void ImapMoveMessagesStrategy::folderAction(ImapStrategyContextBase *context)
{
    // If we're already in a mailbox, we need to close it to expunge the messages
    if ((context->mailbox().isSelected()) && (_lastMailbox.id().isValid())) {
        context->protocol().sendClose();
    } else {
        ImapCopyMessagesStrategy::folderAction(context);
    }
}

void ImapMoveMessagesStrategy::messageAction(ImapStrategyContextBase *context)
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

void ImapMoveMessagesStrategy::completedAction(ImapStrategyContextBase *context)
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

    ImapCopyMessagesStrategy::completedAction(context);
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

    if (source.status() & QMailMessage::Downloaded) {
        message.setStatus(QMailMessage::Downloaded, true);
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

    messageAction(context);
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
    ImapFetchSelectedMessagesStrategy::folderAction(context);
}

void ImapDeleteMessagesStrategy::messageAction(ImapStrategyContextBase *context)
{
    if (selectNextMessageSequence(context, 1000)) {
        QStringList uids;
        foreach (const QString &msgUid, _retrieveUid.split("\n"))
            uids.append(ImapProtocol::uid(msgUid));

        context->protocol().sendUidStore(MFlag_Deleted, IntegerRegion(uids).toString());
    }
}

void ImapDeleteMessagesStrategy::folderAction(ImapStrategyContextBase *context)
{
    // If we're already in a mailbox, we need to close it to expunge the messages
    if ((context->mailbox().isSelected()) && (_lastMailbox.id().isValid())) {
        context->protocol().sendClose();
    } else {
        // Select the next folder, and clear the stored list
        _storedList.clear();
        ImapFetchSelectedMessagesStrategy::folderAction(context);
    }
}

void ImapDeleteMessagesStrategy::completedAction(ImapStrategyContextBase *context)
{
    // If we're already in a mailbox, we need to close it to expunge the messages
    if ((context->mailbox().isSelected()) && (_lastMailbox.id().isValid())) {
        context->protocol().sendClose();
    } else {
        ImapFetchSelectedMessagesStrategy::completedAction(context);
    }
}


ImapStrategyContext::ImapStrategyContext(ImapClient *client)
  : ImapStrategyContextBase(client), 
    _strategy(0) 
{
}
