/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
    SectionProperties(const QMailMessagePart::Location &location = QMailMessagePart::Location(),
                      int minimum = All)
        :  _location(location),
          _minimum(minimum)
    {
    }

    enum MinimumType {
        All = -1
    };

    QMailMessagePart::Location _location;
    int _minimum;
};

typedef QMultiMap<uint, SectionProperties> FolderMap;
typedef QMap<QMailFolderId, FolderMap> SelectionMap;

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
    void operationCompleted();

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
    virtual void downloadSize(ImapStrategyContextBase *context, const QString &uid, int length);

protected:
    virtual void initialAction(ImapStrategyContextBase *context);

    enum TransferState { Init, List, Search, Preview, Complete };

    TransferState _transferState;
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

protected:
    enum { DefaultBatchSize = 50 };

    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleSelect(ImapStrategyContextBase *context);

    virtual void folderAction(ImapStrategyContextBase *context);
    virtual void messageAction(ImapStrategyContextBase *context) = 0;
    virtual void completedAction(ImapStrategyContextBase *context);

    virtual bool computeStartEndPartRange(ImapStrategyContextBase *context);
    virtual bool selectNextMessageSequence(ImapStrategyContextBase *context, int maximum = DefaultBatchSize);

    SelectionMap _selectionMap;
    SelectionMap::ConstIterator _folderItr;
    FolderMap::ConstIterator _selectionItr;
    QMailFolder _currentMailbox;
    QString _retrieveUid;
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

    virtual void messageAction(ImapStrategyContextBase *context);

    virtual void itemFetched(ImapStrategyContextBase *context, const QString &uid);

    uint _headerLimit;
    int _listSize;
    int _messageCount;
    int _messageCountIncremental;

    // RetrievalMap maps uid -> ((units, bytes) to be retrieved, percentage retrieved)
    typedef QMap<QString, QPair< QPair<uint, uint>, uint> > RetrievalMap;
    RetrievalMap _retrievalSize;
    uint _progressRetrievalSize;
    uint _totalRetrievalSize;
};

class ImapFolderListStrategy : public ImapFetchSelectedMessagesStrategy
{
public:
    ImapFolderListStrategy() {}
    virtual ~ImapFolderListStrategy() {}

    virtual void selectedFoldersAppend(const QMailFolderIdList &ids);

    virtual void newConnection(ImapStrategyContextBase *context);
    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);
    
    virtual void mailboxListed(ImapStrategyContextBase *context, QMailFolder& folder, const QString &flags);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleList(ImapStrategyContextBase *context);
    virtual void handleSelect(ImapStrategyContextBase *context);
    virtual void handleSearch(ImapStrategyContextBase *context);

    virtual void listCompleted(ImapStrategyContextBase *context);

    virtual void processMailbox(ImapStrategyContextBase *context);

    virtual void processNextMailbox(ImapStrategyContextBase *context);
    virtual bool getnextMailbox();
    virtual void newfolderAction(ImapStrategyContextBase *context);

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
    virtual void handleSelect(ImapStrategyContextBase *context);
    virtual void handleUidSearch(ImapStrategyContextBase *context);

    virtual bool getnextMailbox();
    virtual void newfolderAction(ImapStrategyContextBase *context);

    virtual void processUidSearchResults(ImapStrategyContextBase *context);

    virtual void listCompleted(ImapStrategyContextBase *context);

private:
    QMailMessageIdList _selectedMessageIds;
    QMap<QMailFolderId, QStringList> _folderMessageUids;
    QMailFolderIdList _monitoredFoldersIds;
    QStringList _serverUids;
    QString _filter;
    enum SearchState { Seen, Unseen };
    SearchState _searchState;

    QStringList _seenUids;
    QStringList _unseenUids;
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
    virtual bool selectNextMailbox(ImapStrategyContextBase *context);
    virtual bool nextMailbox(ImapStrategyContextBase *context);

    virtual void fetchNextMailPreview(ImapStrategyContextBase *context);

    virtual void processUidSearchResults(ImapStrategyContextBase *context);

    virtual void recursivelyCompleteParts(ImapStrategyContextBase *context, const QMailMessagePartContainer &partContainer, int &partsToRetrieve, int &bytesLeft);

protected:
    QStringList _newUids;
    QList<QPair<QMailFolderId, QStringList> > _retrieveUids;
    QMailMessageIdList _completionList;
    QList<QPair<QMailMessagePart::Location, uint> > _completionSectionList;

private:
    uint _progress;
    uint _total;
};

class ImapRetrieveFolderListStrategy : public ImapSynchronizeBaseStrategy
{
public:
    ImapRetrieveFolderListStrategy() :_descending(false) {}
    virtual ~ImapRetrieveFolderListStrategy() {}

    virtual void setBase(const QMailFolderId &folderId);
    virtual void setDescending(bool descending);

    virtual void newConnection(ImapStrategyContextBase *context);
    
    virtual void mailboxListed(ImapStrategyContextBase *context, QMailFolder& folder, const QString &flags);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleSearch(ImapStrategyContextBase *context);

    virtual void listCompleted(ImapStrategyContextBase *context);

    virtual void processNextMailbox(ImapStrategyContextBase *context);

    void removeDeletedMailboxes(ImapStrategyContextBase *context);

    QMailFolderId _baseId;
    bool _descending;
    QStringList _mailboxPaths;
    QMailFolderIdList _mailboxList;
};

class ImapRetrieveMessageListStrategy : public ImapSynchronizeBaseStrategy
{
public:
    ImapRetrieveMessageListStrategy() {}
    virtual ~ImapRetrieveMessageListStrategy() {}

    virtual void setMinimum(uint minimum);

    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleUidSearch(ImapStrategyContextBase *context);

    virtual void completedAction(ImapStrategyContextBase *context);
    virtual void listCompleted(ImapStrategyContextBase *context);
    virtual void processUidSearchResults(ImapStrategyContextBase *context);

    virtual void processMailbox(ImapStrategyContextBase *context);

    uint _minimum;
    bool _fillingGap;
    QMap<QMailFolderId, IntegerRegion> _newMinMaxMap;
    QMap<QMailFolderId, int> _lastExistsMap;
    QMap<QMailFolderId, int> _lastUidNextMap;
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

    virtual void processMailbox(ImapStrategyContextBase *context);

    virtual void processUidSearchResults(ImapStrategyContextBase *context);
    virtual void searchInconclusive(ImapStrategyContextBase *context);

    virtual void listCompleted(ImapStrategyContextBase *context);

    virtual bool setNextSeen(ImapStrategyContextBase *context);
    virtual bool setNextDeleted(ImapStrategyContextBase *context);

protected:
    QStringList _readUids;
    QStringList _removedUids;
    bool _expungeRequired;

private:
    Options _options;

    enum SearchState { All, Seen, Unseen, Inconclusive };
    SearchState _searchState;

    QStringList _seenUids;
    QStringList _unseenUids;
};

class ImapRetrieveAllStrategy : public ImapSynchronizeAllStrategy
{
public:
    ImapRetrieveAllStrategy();
    virtual ~ImapRetrieveAllStrategy() {}
};

class ImapExportUpdatesStrategy : public ImapSynchronizeAllStrategy
{
public:
    ImapExportUpdatesStrategy();
    virtual ~ImapExportUpdatesStrategy() {}

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleSelect(ImapStrategyContextBase *context);
    virtual void handleUidSearch(ImapStrategyContextBase *context);
    virtual void processUidSearchResults(ImapStrategyContextBase *context);

    QStringList _serverReportedUids;
    QStringList _clientDeletedUids;
    QStringList _clientReadUids;
};

class ImapCopyMessagesStrategy : public ImapFetchSelectedMessagesStrategy
{
public:
    ImapCopyMessagesStrategy() {}
    virtual ~ImapCopyMessagesStrategy() {}

    void setDestination(const QMailFolderId &destinationId);

    virtual void newConnection(ImapStrategyContextBase *context);
    virtual void transition(ImapStrategyContextBase*, const ImapCommand, const OperationStatus);
    
    virtual void messageCopied(ImapStrategyContextBase *context, const QString &copiedUid, const QString &createdUid);
    virtual void messageFetched(ImapStrategyContextBase *context, QMailMessage &message);

protected:
    virtual void handleLogin(ImapStrategyContextBase *context);
    virtual void handleSelect(ImapStrategyContextBase *context);
    virtual void handleUidCopy(ImapStrategyContextBase *context);
    virtual void handleUidSearch(ImapStrategyContextBase *context);
    virtual void handleUidFetch(ImapStrategyContextBase *context);

    virtual void messageAction(ImapStrategyContextBase *context);
    virtual void completedAction(ImapStrategyContextBase *context);

    virtual void updateCopiedMessage(ImapStrategyContextBase *context, QMailMessage &message, const QMailMessage &source);

    virtual void fetchNextCopy(ImapStrategyContextBase *context);

    QMailFolder _destination;
    QMap<QString, QString> _sourceUid;
    QStringList _sourceUids;
    int _sourceIndex;
    //QString _uidNext;
    QStringList _createdUids;
    int _messagesAdded;
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

    virtual void folderAction(ImapStrategyContextBase *context);
    virtual void messageAction(ImapStrategyContextBase *context);
    virtual void completedAction(ImapStrategyContextBase *context);

    virtual void updateCopiedMessage(ImapStrategyContextBase *context, QMailMessage &message, const QMailMessage &source);

    QMailFolder _lastMailbox;
};

class ImapDeleteMessagesStrategy : public ImapFetchSelectedMessagesStrategy
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

    virtual void messageAction(ImapStrategyContextBase *context);
    virtual void folderAction(ImapStrategyContextBase *context);
    virtual void completedAction(ImapStrategyContextBase *context);

    QStringList _storedList;
    bool _removal;
    QMailFolder _lastMailbox;
};

class ImapStrategyContext : public ImapStrategyContextBase
{
public:
    ImapStrategyContext(ImapClient *client);

    ImapFetchSelectedMessagesStrategy selectedStrategy;
    ImapRetrieveFolderListStrategy foldersOnlyStrategy;
    ImapExportUpdatesStrategy exportUpdatesStrategy;
    ImapUpdateMessagesFlagsStrategy updateMessagesFlagsStrategy;
    ImapSynchronizeAllStrategy synchronizeAccountStrategy;
    ImapCopyMessagesStrategy copyMessagesStrategy;
    ImapMoveMessagesStrategy moveMessagesStrategy;
    ImapDeleteMessagesStrategy deleteMessagesStrategy;
    ImapRetrieveMessageListStrategy retrieveMessageListStrategy;
    ImapRetrieveAllStrategy retrieveAllStrategy;

    void newConnection() { _strategy->newConnection(this); }
    void commandTransition(const ImapCommand command, const OperationStatus status) { _strategy->transition(this, command, status); }

    void mailboxListed(QMailFolder& folder, const QString &flags) { _strategy->mailboxListed(this, folder, flags); }
    void messageFetched(QMailMessage &message) { _strategy->messageFetched(this, message); } 
    void dataFetched(QMailMessage &message, const QString &uid, const QString &section) { _strategy->dataFetched(this, message, uid, section); } 
    void nonexistentUid(const QString &uid) { _strategy->nonexistentUid(this, uid); } 
    void messageStored(const QString &uid) { _strategy->messageStored(this, uid); }
    void messageCopied(const QString &copiedUid, const QString &createdUid) { _strategy->messageCopied(this, copiedUid, createdUid); }
    void downloadSize(const QString &uid, int length) { _strategy->downloadSize(this, uid, length); }

    ImapStrategy *strategy() const { return _strategy; }
    void setStrategy(ImapStrategy *strategy) { _strategy = strategy; }

private:
    ImapStrategy *_strategy;
};

#endif
