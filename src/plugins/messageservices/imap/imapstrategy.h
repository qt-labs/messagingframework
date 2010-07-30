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

#ifndef IMAPSTRATEGY_H
#define IMAPSTRATEGY_H

#include "imapprotocol.h"
#include "integerregion.h"
#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>
#include <qmap.h>
#include <qmailfolder.h>


struct SectionProperties {
    enum MinimumType {
        All = -1
    };

    SectionProperties(const QMailMessagePart::Location &location = QMailMessagePart::Location(),
                      int minimum = All)
        :  _location(location),
          _minimum(minimum)
    {
    }

    bool isEmpty() const
    {
        return (!_location.isValid() && (_minimum == All));
    }

    QMailMessagePart::Location _location;
    int _minimum;
};

struct MessageSelector
{
    MessageSelector(uint uid, const QMailMessageId &messageId, const SectionProperties &properties)
        : _uid(uid),
          _messageId(messageId),
          _properties(properties)
    {
    }

    QString uidString(const QString &mailbox) const
    {
        if (_uid == 0) {
            return ("id:" + QString::number(_messageId.toULongLong()));
        } else {
            return (mailbox + QString::number(_uid));
        }
    }

    uint _uid;
    QMailMessageId _messageId;
    SectionProperties _properties;
};

typedef QList<MessageSelector> FolderSelections;
typedef QMap<QMailFolderId, FolderSelections> SelectionMap;

class QMailAccount;
class QMailAccountConfiguration;
class ImapClient;

class ImapStrategyContextBase
{
public:
    ImapStrategyContextBase(ImapClient *client) { _client = client; }
    virtual ~ImapStrategyContextBase()  {}

    ImapClient *client();
    ImapProtocol &protocol();
    const ImapMailboxProperties &mailbox();
    const QMailAccountConfiguration &config();

    void folderModified(const QMailFolderId &folderId) { _modifiedFolders.insert(folderId); }

    void updateStatus(const QString &);
    void progressChanged(uint, uint);
    void completedMessageAction(const QString &uid);
    void completedMessageCopy(QMailMessage &message, const QMailMessage &original);
    void operationCompleted();
    void matchingMessageIds(const QMailMessageIdList &msgs);

private:
    ImapClient *_client;
    QSet<QMailFolderId> _modifiedFolders;
};

class ImapStrategy
{
public:
    ImapStrategy() {}
    virtual ~ImapStrategy() {}
    
    virtual void newConnection(ImapStrategyContextBase *context);
    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus) = 0;

    virtual void mailboxListed(ImapStrategyContextBase *context, QMailFolder& folder, const QString &flags);
    virtual void messageFetched(ImapStrategyContextBase *context, QMailMessage &message);
    virtual void dataFetched(ImapStrategyContextBase *context, QMailMessage &message, const QString &uid, const QString &section);
    virtual void nonexistentUid(ImapStrategyContextBase *context, const QString &uid);
    virtual void messageStored(ImapStrategyContextBase *context, const QString &uid);
    virtual void messageCopied(ImapStrategyContextBase *context, const QString &copiedUid, const QString &createdUid);
    virtual void messageCreated(ImapStrategyContextBase *context, const QMailMessageId &id, const QString &uid);
    virtual void downloadSize(ImapStrategyContextBase *context, const QString &uid, int length);
    virtual void urlAuthorized(ImapStrategyContextBase *context, const QString &url);
    virtual void folderCreated(ImapStrategyContextBase *context, const QString &folder);
    virtual void folderDeleted(ImapStrategyContextBase *context, const QMailFolder &folder);
    virtual void folderRenamed(ImapStrategyContextBase *context, const QMailFolder &folder, const QString &newName);

    void clearError() { _error = false; }
    bool error() { return _error; }

protected:
    virtual void initialAction(ImapStrategyContextBase *context);

    enum TransferState { Init, List, Search, Preview, Complete, Copy };

    TransferState _transferState;
    QString _baseFolder;
    bool _error;
};

class ImapCreateFolderStrategy : public ImapStrategy
{
public:
    ImapCreateFolderStrategy(): _inProgress(0) { }
    virtual ~ImapCreateFolderStrategy() {}

    virtual void transition(ImapStrategyContextBase *, const ImapCommand, const OperationStatus);
    virtual void createFolder(const QMailFolderId &folder, const QString &name);
    virtual void folderCreated(ImapStrategyContextBase *context, const QString &folder);
protected:
    virtual void handleCreate(ImapStrategyContextBase *context);
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void process(ImapStrategyContextBase *context);

    QList<QPair<QMailFolderId, QString> > _folders;
    int _inProgress;
};


class ImapDeleteFolderStrategy : public ImapStrategy
{
public:
    ImapDeleteFolderStrategy() : _inProgress(0) { }
    virtual ~ImapDeleteFolderStrategy() {}

    virtual void transition(ImapStrategyContextBase *, const ImapCommand, const OperationStatus);
    virtual void deleteFolder(const QMailFolderId &folderId);
    virtual void folderDeleted(ImapStrategyContextBase *context, const QMailFolder &folder);
protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleDelete(ImapStrategyContextBase *context);
    virtual void process(ImapStrategyContextBase *context);
    virtual void deleteFolder(const QMailFolderId &folderId, ImapStrategyContextBase *context);
    QList<QMailFolderId> _folderIds;
    int _inProgress;
};

class ImapRenameFolderStrategy : public ImapStrategy
{
public:
    ImapRenameFolderStrategy() : _inProgress(0) { }
    virtual ~ImapRenameFolderStrategy() {}

    virtual void transition(ImapStrategyContextBase *, const ImapCommand, const OperationStatus);
    virtual void renameFolder(const QMailFolderId &folderId, const QString &newName);
    virtual void folderRenamed(ImapStrategyContextBase *context, const QMailFolder &folder, const QString &name);
protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleRename(ImapStrategyContextBase *context);
    virtual void process(ImapStrategyContextBase *context);
    QList<QPair<QMailFolderId, QString> > _folderNewNames;
    int _inProgress;
};

class ImapPrepareMessagesStrategy : public ImapStrategy
{
public:
    ImapPrepareMessagesStrategy() {}
    virtual ~ImapPrepareMessagesStrategy() {}

    virtual void setUnresolved(const QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > &ids, bool external);

    virtual void newConnection(ImapStrategyContextBase *context);
    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);

    virtual void urlAuthorized(ImapStrategyContextBase *context, const QString &url);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleGenUrlAuth(ImapStrategyContextBase *context);

    virtual void nextMessageAction(ImapStrategyContextBase *context);
    virtual void messageListCompleted(ImapStrategyContextBase *context);

    QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > _locations;
    bool _external;
};
    
class ImapMessageListStrategy : public ImapStrategy
{
public:
    ImapMessageListStrategy() {}
    virtual ~ImapMessageListStrategy() {}

    virtual void clearSelection();
    virtual void selectedMailsAppend(const QMailMessageIdList &ids);
    virtual void selectedSectionsAppend(const QMailMessagePart::Location &location);
    
    virtual void newConnection(ImapStrategyContextBase *context);
    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);

    virtual void initialAction(ImapStrategyContextBase *context);

protected:
    enum { DefaultBatchSize = 50 };

    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleSelect(ImapStrategyContextBase *context);
    virtual void handleCreate(ImapStrategyContextBase *context);
    virtual void handleDelete(ImapStrategyContextBase *context);
    virtual void handleRename(ImapStrategyContextBase *context);
    virtual void handleClose(ImapStrategyContextBase *context);

    virtual void messageListFolderAction(ImapStrategyContextBase *context);
    virtual void messageListMessageAction(ImapStrategyContextBase *context) = 0;
    virtual void messageListCompleted(ImapStrategyContextBase *context);

    virtual void resetMessageListTraversal();
    virtual bool selectNextMessageSequence(ImapStrategyContextBase *context, int maximum = DefaultBatchSize, bool folderActionPermitted = true);

    virtual void setCurrentMailbox(const QMailFolderId &id);

    virtual bool messageListFolderActionRequired();

    SelectionMap _selectionMap;
    SelectionMap::Iterator _folderItr;
    FolderSelections::ConstIterator _selectionItr;
    QMailFolder _currentMailbox;
    QString _currentModSeq;
    QStringList _messageUids;
    QMailMessagePart::Location _msgSection;
    int _sectionStart;
    int _sectionEnd;
};

class ImapFetchSelectedMessagesStrategy : public ImapMessageListStrategy
{
public:
    ImapFetchSelectedMessagesStrategy() {}
    virtual ~ImapFetchSelectedMessagesStrategy() {}
    
    virtual void setOperation(QMailRetrievalAction::RetrievalSpecification spec);
    virtual void clearSelection();
    virtual void selectedMailsAppend(const QMailMessageIdList &ids);
    virtual void selectedSectionsAppend(const QMailMessagePart::Location &, int = -1);

    virtual void newConnection(ImapStrategyContextBase *context);
    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);

    virtual void downloadSize(ImapStrategyContextBase*, const QString &uid, int length);
    virtual void messageFetched(ImapStrategyContextBase *context, QMailMessage &message);
    virtual void dataFetched(ImapStrategyContextBase *context, QMailMessage &message, const QString &uid, const QString &section);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleUidFetch(ImapStrategyContextBase *context);

    virtual void messageListMessageAction(ImapStrategyContextBase *context);

    virtual void itemFetched(ImapStrategyContextBase *context, const QString &uid);

    uint _headerLimit;
    int _listSize;
    int _messageCount;
    int _messageCountIncremental;
    int _outstandingFetches;

    // RetrievalMap maps uid -> ((units, bytes) to be retrieved, percentage retrieved)
    typedef QMultiMap<QString, QPair< QPair<uint, uint>, uint> > RetrievalMap;
    RetrievalMap _retrievalSize;
    uint _progressRetrievalSize;
    uint _totalRetrievalSize;
};

class ImapFolderListStrategy : public ImapFetchSelectedMessagesStrategy
{
public:
    ImapFolderListStrategy() : _processed(0), _processable(0) {}
    virtual ~ImapFolderListStrategy() {}

    virtual void clearSelection();
    virtual void selectedFoldersAppend(const QMailFolderIdList &ids);

    virtual void newConnection(ImapStrategyContextBase *context);
    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);

    virtual void mailboxListed(ImapStrategyContextBase *context, QMailFolder& folder, const QString &flags);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleList(ImapStrategyContextBase *context);
    virtual void handleSelect(ImapStrategyContextBase *context);
    virtual void handleSearch(ImapStrategyContextBase *context);

    virtual void folderListFolderAction(ImapStrategyContextBase *context);
    virtual void folderListCompleted(ImapStrategyContextBase *context);

    virtual void processNextFolder(ImapStrategyContextBase *context);
    virtual bool nextFolder();
    virtual void processFolder(ImapStrategyContextBase *context);

    void updateUndiscoveredCount(ImapStrategyContextBase *context);

    enum FolderStatus
    {
        NoInferiors = (1 << 0),
        NoSelect = (1 << 1),
        Marked = (1 << 2),
        Unmarked = (1 << 3),
        HasChildren = (1 << 4),
        HasNoChildren = (1 << 5)
    };

    QMailFolderIdList _mailboxIds;
    QMap<QMailFolderId, FolderStatus> _folderStatus;

    int _processed;
    int _processable;
};

class ImapUpdateMessagesFlagsStrategy : public ImapFolderListStrategy 
{
public:
    ImapUpdateMessagesFlagsStrategy() {}
    virtual ~ImapUpdateMessagesFlagsStrategy() {}

    virtual void clearSelection();      
    virtual void selectedMailsAppend(const QMailMessageIdList &messageIds);

    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleUidSearch(ImapStrategyContextBase *context);

    virtual void folderListFolderAction(ImapStrategyContextBase *context);
    virtual void folderListCompleted(ImapStrategyContextBase *context);

    virtual bool nextFolder();
    virtual void processFolder(ImapStrategyContextBase *context);

    virtual void processUidSearchResults(ImapStrategyContextBase *context);

private:
    QMailMessageIdList _selectedMessageIds;
    QMap<QMailFolderId, QStringList> _folderMessageUids;
    QStringList _serverUids;
    QString _filter;
    enum SearchState { Seen, Unseen, Flagged };
    SearchState _searchState;

    QStringList _seenUids;
    QStringList _unseenUids;
    QStringList _flaggedUids;
};

class ImapSynchronizeBaseStrategy : public ImapFolderListStrategy 
{
public:
    ImapSynchronizeBaseStrategy() {}
    virtual ~ImapSynchronizeBaseStrategy() {}

    virtual void newConnection(ImapStrategyContextBase *context);
    
    virtual void messageFetched(ImapStrategyContextBase *context, QMailMessage &message);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleSelect(ImapStrategyContextBase *context);
    virtual void handleUidFetch(ImapStrategyContextBase *context);

    virtual void previewDiscoveredMessages(ImapStrategyContextBase *context);
    virtual bool selectNextPreviewFolder(ImapStrategyContextBase *context);

    virtual void fetchNextMailPreview(ImapStrategyContextBase *context);
    virtual void folderPreviewCompleted(ImapStrategyContextBase *context);

    virtual void processUidSearchResults(ImapStrategyContextBase *context);

    virtual void recursivelyCompleteParts(ImapStrategyContextBase *context, const QMailMessagePartContainer &partContainer, int &partsToRetrieve, int &bytesLeft);

protected:
    QStringList _newUids;
    QList<QPair<QMailFolderId, QStringList> > _retrieveUids;
    QMailMessageIdList _completionList;
    QList<QPair<QMailMessagePart::Location, uint> > _completionSectionList;
    int _outstandingPreviews;

private:
    uint _progress;
    uint _total;
};

class ImapRetrieveFolderListStrategy : public ImapSynchronizeBaseStrategy
{
public:
    ImapRetrieveFolderListStrategy() : _descending(false) {}
    virtual ~ImapRetrieveFolderListStrategy() {}

    virtual void setBase(const QMailFolderId &folderId);
    virtual void setDescending(bool descending);

    virtual void newConnection(ImapStrategyContextBase *context);

    virtual void mailboxListed(ImapStrategyContextBase *context, QMailFolder& folder, const QString &flags);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleSearch(ImapStrategyContextBase *context);
    virtual void handleList(ImapStrategyContextBase *context);

    virtual void folderListCompleted(ImapStrategyContextBase *context);

    void removeDeletedMailboxes(ImapStrategyContextBase *context);

    QMailFolderId _baseId;
    bool _descending;
    QStringList _mailboxPaths;
    QSet<QString> _ancestorPaths;
    QStringList _ancestorSearchPaths;
    QMailFolderIdList _mailboxList;
};

class ImapRetrieveMessageListStrategy : public ImapSynchronizeBaseStrategy
{
public:
    ImapRetrieveMessageListStrategy() {}
    virtual ~ImapRetrieveMessageListStrategy() {}

    virtual void setMinimum(uint minimum);
    virtual void setAccountCheck(bool accountCheck);

    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleUidSearch(ImapStrategyContextBase *context);

    virtual void messageListCompleted(ImapStrategyContextBase *context);
    virtual void folderListCompleted(ImapStrategyContextBase *context);
    virtual void processUidSearchResults(ImapStrategyContextBase *context);

    virtual void folderListFolderAction(ImapStrategyContextBase *context);

    uint _minimum;
    bool _accountCheck;
    bool _fillingGap;
    bool _listAll;
    QMap<QMailFolderId, IntegerRegion> _newMinMaxMap;
    QMailFolderIdList _updatedFolders;
};

class ImapSynchronizeAllStrategy : public ImapRetrieveFolderListStrategy
{
public:
    enum Options
    {
        RetrieveMail = (1 << 0),
        ImportChanges = (1 << 1),
        ExportChanges = (1 << 2)
    };

    ImapSynchronizeAllStrategy();
    virtual ~ImapSynchronizeAllStrategy() {}
    
    void setOptions(Options options);

    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);

protected:
    virtual void handleUidSearch(ImapStrategyContextBase *context);
    virtual void handleUidStore(ImapStrategyContextBase *context);
    virtual void handleExpunge(ImapStrategyContextBase *context);

    virtual void folderListFolderAction(ImapStrategyContextBase *context);

    virtual void processUidSearchResults(ImapStrategyContextBase *context);
    virtual void searchInconclusive(ImapStrategyContextBase *context);

    virtual void folderListCompleted(ImapStrategyContextBase *context);

    virtual bool setNextSeen(ImapStrategyContextBase *context);
    virtual bool setNextNotSeen(ImapStrategyContextBase *context);
    virtual bool setNextImportant(ImapStrategyContextBase *context);
    virtual bool setNextNotImportant(ImapStrategyContextBase *context);
    virtual bool setNextDeleted(ImapStrategyContextBase *context);

    virtual void folderPreviewCompleted(ImapStrategyContextBase *context);

protected:
    QStringList _readUids;
    QStringList _unreadUids;
    QStringList _importantUids;
    QStringList _unimportantUids;
    QStringList _removedUids;
    QStringList _storedReadUids;
    QStringList _storedUnreadUids;
    QStringList _storedImportantUids;
    QStringList _storedUnimportantUids;
    QStringList _storedRemovedUids;
    bool _expungeRequired;
    static const int batchSize = 1000;

private:
    Options _options;

    enum SearchState { All, Seen, Unseen, Flagged, Inconclusive };
    SearchState _searchState;

    QStringList _seenUids;
    QStringList _unseenUids;
    QStringList _flaggedUids;
};

class ImapRetrieveAllStrategy : public ImapSynchronizeAllStrategy
{
public:
    ImapRetrieveAllStrategy();
    virtual ~ImapRetrieveAllStrategy() {}
};

class ImapSearchMessageStrategy : public ImapRetrieveFolderListStrategy
{
public:
    ImapSearchMessageStrategy() : _canceled(false) { setBase(QMailFolderId()); setDescending(true); }
    virtual ~ImapSearchMessageStrategy() {}

    virtual void cancelSearch();
    virtual void searchArguments(const QMailMessageKey &searchCriteria, const QString &bodyText, const QMailMessageSortKey &sort);
    virtual void transition(ImapStrategyContextBase *, const ImapCommand, const OperationStatus);
protected:
    virtual void handleSearchMessage(ImapStrategyContextBase *context);
    virtual void handleUidFetch(ImapStrategyContextBase *context);
    virtual void folderListFolderAction(ImapStrategyContextBase *context);

    virtual void messageFetched(ImapStrategyContextBase *context, QMailMessage &message);
    virtual void messageListCompleted(ImapStrategyContextBase *context);

    struct SearchData {
        QMailMessageKey criteria;
        QString bodyText;
        QMailMessageSortKey sort;
    };
    QList<SearchData> _searches;
    QList<QMailMessageId> _fetchedList;
    bool _canceled;
};

class ImapExportUpdatesStrategy : public ImapSynchronizeAllStrategy
{
public:
    ImapExportUpdatesStrategy();
    virtual ~ImapExportUpdatesStrategy() {}

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleUidSearch(ImapStrategyContextBase *context);

    virtual void folderListFolderAction(ImapStrategyContextBase *context);
    virtual bool nextFolder();

    virtual void processUidSearchResults(ImapStrategyContextBase *context);

    QStringList _serverReportedUids;
    QStringList _clientDeletedUids;
    QStringList _clientReadUids;
    QStringList _clientUnreadUids;
    QStringList _clientImportantUids;
    QStringList _clientUnimportantUids;

    QMap<QMailFolderId, QList<QStringList> > _folderMessageUids;
};

class ImapCopyMessagesStrategy : public ImapFetchSelectedMessagesStrategy
{
public:
    ImapCopyMessagesStrategy() {}
    virtual ~ImapCopyMessagesStrategy() {}

    virtual void clearSelection();
    virtual void appendMessageSet(const QMailMessageIdList &ids, const QMailFolderId &destinationId);

    virtual void newConnection(ImapStrategyContextBase *context);
    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);
    
    virtual void messageCopied(ImapStrategyContextBase *context, const QString &copiedUid, const QString &createdUid);
    virtual void messageCreated(ImapStrategyContextBase *context, const QMailMessageId &id, const QString &createdUid);
    virtual void messageFetched(ImapStrategyContextBase *context, QMailMessage &message);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleSelect(ImapStrategyContextBase *context);
    virtual void handleUidCopy(ImapStrategyContextBase *context);
    virtual void handleAppend(ImapStrategyContextBase *context);
    virtual void handleUidSearch(ImapStrategyContextBase *context);
    virtual void handleUidStore(ImapStrategyContextBase *context);
    virtual void handleUidFetch(ImapStrategyContextBase *context);

    virtual void messageListFolderAction(ImapStrategyContextBase *context);
    virtual void messageListMessageAction(ImapStrategyContextBase *context);
    virtual void messageListCompleted(ImapStrategyContextBase *context);

    virtual QString copiedMessageFetched(ImapStrategyContextBase *context, QMailMessage &message);
    virtual void updateCopiedMessage(ImapStrategyContextBase *context, QMailMessage &message, const QMailMessage &source);

    virtual void removeObsoleteUids(ImapStrategyContextBase *context);

    virtual void copyNextMessage(ImapStrategyContextBase *context);
    virtual void fetchNextCopy(ImapStrategyContextBase *context);

    virtual void selectMessageSet(ImapStrategyContextBase *context);

    QList<QPair<QMailMessageIdList, QMailFolderId> > _messageSets;
    QMailFolder _destination;
    QMap<QString, QString> _sourceUid;
    QStringList _sourceUids;
    int _sourceIndex;
    QStringList _createdUids;
    int _messagesAdded;
    QStringList _obsoleteDestinationUids;
};

class ImapMoveMessagesStrategy : public ImapCopyMessagesStrategy
{
public:
    ImapMoveMessagesStrategy() {}
    virtual ~ImapMoveMessagesStrategy() {}

    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);

protected:
    virtual void handleUidCopy(ImapStrategyContextBase *context);
    virtual void handleUidStore(ImapStrategyContextBase *context);
    virtual void handleClose(ImapStrategyContextBase *context);
    virtual void handleUidFetch(ImapStrategyContextBase *context);
    virtual void handleExamine(ImapStrategyContextBase *context);

    virtual void messageListFolderAction(ImapStrategyContextBase *context);
    virtual void messageListMessageAction(ImapStrategyContextBase *context);
    virtual void messageListCompleted(ImapStrategyContextBase *context);

    virtual void updateCopiedMessage(ImapStrategyContextBase *context, QMailMessage &message, const QMailMessage &source);

    QMailFolder _lastMailbox;
};

class ImapExternalizeMessagesStrategy : public ImapCopyMessagesStrategy
{
public:
    ImapExternalizeMessagesStrategy() {}
    virtual ~ImapExternalizeMessagesStrategy() {}

    virtual void appendMessageSet(const QMailMessageIdList &ids, const QMailFolderId &destinationId);

    virtual void newConnection(ImapStrategyContextBase *context);
    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);

    virtual void messageFetched(ImapStrategyContextBase *context, QMailMessage &message);
    virtual void urlAuthorized(ImapStrategyContextBase *context, const QString &url);

protected:
    virtual void handleGenUrlAuth(ImapStrategyContextBase *context);

    virtual void messageListCompleted(ImapStrategyContextBase *context);

    virtual void updateCopiedMessage(ImapStrategyContextBase *context, QMailMessage &message, const QMailMessage &source);

    virtual void resolveNextMessage(ImapStrategyContextBase *context);

    QMailMessageIdList _urlIds;
};

class ImapFlagMessagesStrategy : public ImapFetchSelectedMessagesStrategy
{
public:
    ImapFlagMessagesStrategy() {}
    virtual ~ImapFlagMessagesStrategy() {}

    virtual void newConnection(ImapStrategyContextBase *context);
    virtual void clearSelection();
    virtual void setMessageFlags(MessageFlags flags, bool set);

    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);
    
protected:
    virtual void handleUidStore(ImapStrategyContextBase *context);

    virtual void messageListMessageAction(ImapStrategyContextBase *context);

    MessageFlags _setMask;
    MessageFlags _unsetMask;
    int _outstandingStores;
};

class ImapDeleteMessagesStrategy : public ImapFlagMessagesStrategy
{
public:
    ImapDeleteMessagesStrategy() {}
    virtual ~ImapDeleteMessagesStrategy() {}

    void setLocalMessageRemoval(bool removal);

    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);
    
protected:
    virtual void handleUidStore(ImapStrategyContextBase *context);
    virtual void handleClose(ImapStrategyContextBase *context);
    virtual void handleExamine(ImapStrategyContextBase *context);

    virtual void messageListFolderAction(ImapStrategyContextBase *context);
    virtual void messageListCompleted(ImapStrategyContextBase *context);

    bool _removal;
    QStringList _storedList;
    QMailFolder _lastMailbox;
};

class ImapStrategyContext : public ImapStrategyContextBase
{
public:
    ImapStrategyContext(ImapClient *client);

    ImapPrepareMessagesStrategy prepareMessagesStrategy;
    ImapFetchSelectedMessagesStrategy selectedStrategy;
    ImapRetrieveFolderListStrategy foldersOnlyStrategy;
    ImapExportUpdatesStrategy exportUpdatesStrategy;
    ImapUpdateMessagesFlagsStrategy updateMessagesFlagsStrategy;
    ImapSynchronizeAllStrategy synchronizeAccountStrategy;
    ImapCopyMessagesStrategy copyMessagesStrategy;
    ImapMoveMessagesStrategy moveMessagesStrategy;
    ImapExternalizeMessagesStrategy externalizeMessagesStrategy;
    ImapFlagMessagesStrategy flagMessagesStrategy;
    ImapDeleteMessagesStrategy deleteMessagesStrategy;
    ImapRetrieveMessageListStrategy retrieveMessageListStrategy;
    ImapRetrieveAllStrategy retrieveAllStrategy;
    ImapCreateFolderStrategy createFolderStrategy;
    ImapDeleteFolderStrategy deleteFolderStrategy;
    ImapRenameFolderStrategy renameFolderStrategy;
    ImapSearchMessageStrategy searchMessageStrategy;

    void newConnection() { _strategy->clearError(); _strategy->newConnection(this); }
    void commandTransition(const ImapCommand command, const OperationStatus status) { _strategy->transition(this, command, status); }

    void mailboxListed(QMailFolder& folder, const QString &flags) { _strategy->mailboxListed(this, folder, flags); }
    void messageFetched(QMailMessage &message) { _strategy->messageFetched(this, message); }
    void dataFetched(QMailMessage &message, const QString &uid, const QString &section) { _strategy->dataFetched(this, message, uid, section); }
    void nonexistentUid(const QString &uid) { _strategy->nonexistentUid(this, uid); }
    void messageStored(const QString &uid) { _strategy->messageStored(this, uid); }
    void messageCopied(const QString &copiedUid, const QString &createdUid) { _strategy->messageCopied(this, copiedUid, createdUid); }
    void messageCreated(const QMailMessageId &id, const QString &uid) { _strategy->messageCreated(this, id, uid); }
    void downloadSize(const QString &uid, int length) { _strategy->downloadSize(this, uid, length); }
    void urlAuthorized(const QString &url) { _strategy->urlAuthorized(this, url); }
    void folderCreated(const QString &folder) { _strategy->folderCreated(this, folder); }
    void folderDeleted(const QMailFolder &folder) { _strategy->folderDeleted(this, folder); }
    void folderRenamed(const QMailFolder &folder, const QString &name) { _strategy->folderRenamed(this, folder, name); }

    ImapStrategy *strategy() const { return _strategy; }
    void setStrategy(ImapStrategy *strategy) { _strategy = strategy; }

private:
    ImapStrategy *_strategy;
};

#endif
