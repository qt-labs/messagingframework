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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QMAILSERVICEACTION_P_H
#define QMAILSERVICEACTION_P_H

#include <QSharedPointer>

#include "qmailserviceaction.h"
#include "qmailmessageserver.h"

// These classes are implemented via qmailmessage.cpp and qmailinstantiations.cpp

class QMailServiceActionCommand
{
public:
    virtual ~QMailServiceActionCommand() {};
    virtual void execute() = 0;
};

struct ActionCommand {
    QMailServiceAction *action;
    QSharedPointer<QMailServiceActionCommand> command;
};

class QMailServiceActionPrivate : public QObject, public QPrivateNoncopyableBase
{
    Q_OBJECT

public:
    template<typename Subclass>
    QMailServiceActionPrivate(Subclass *p, QMailServiceAction *i);

    virtual ~QMailServiceActionPrivate();

    void cancelOperation();
    void setAction(quint64 action);

protected slots:
    void activityChanged(quint64, QMailServiceAction::Activity activity);
    void connectivityChanged(quint64, QMailServiceAction::Connectivity connectivity);
    void statusChanged(quint64, const QMailServiceAction::Status status);
    void progressChanged(quint64, uint progress, uint total);
    void subActionConnectivityChanged(QMailServiceAction::Connectivity c);
    void subActionActivityChanged(QMailServiceAction::Activity a);
    void subActionStatusChanged(const QMailServiceAction::Status &s);
    void subActionProgressChanged(uint value, uint total);
    void connectSubAction(QMailServiceAction *subAction);
    void disconnectSubAction(QMailServiceAction *subAction);
    void clearSubActions();
    void serverFailure();

protected:
    friend class QMailServiceAction;

    virtual void init();

    quint64 newAction();
    bool validAction(quint64 action);

    void appendSubAction(QMailServiceAction *subAction, QSharedPointer<QMailServiceActionCommand> command);
    void executeNextSubAction();

    void setActivity(QMailServiceAction::Activity newActivity);
    void setConnectivity(QMailServiceAction::Connectivity newConnectivity);

    void setStatus(const QMailServiceAction::Status &status);
    void setStatus(QMailServiceAction::Status::ErrorCode code, const QString &text);
    void setStatus(QMailServiceAction::Status::ErrorCode code, const QString &text, const QMailAccountId &accountId, const QMailFolderId &folderId, const QMailMessageId &messageId);

    void setProgress(uint newProgress, uint newTotal);

    void emitChanges();

    QMailServiceAction *_interface;
    QMailMessageServer *_server;

    QMailServiceAction::Connectivity _connectivity;
    QMailServiceAction::Activity _activity;
    QMailServiceAction::Status _status;

    uint _total;
    uint _progress;

    bool _isValid;
    quint64 _action;

    bool _connectivityChanged;
    bool _activityChanged;
    bool _progressChanged;
    bool _statusChanged;

    QList<ActionCommand> _pendingActions;
};


class QMailRetrievalActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailRetrievalActionPrivate(QMailRetrievalAction *);

    void retrieveFolderListHelper(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending = true);
    void retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageListHelper(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);
    void retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);
    void retrieveMessageLists(const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort);
    void retrieveNewMessages(const QMailAccountId &accountId, const QMailFolderIdList &folderIds);

    void createStandardFolders(const QMailAccountId &accountId);

    void retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum);

    void exportUpdatesHelper(const QMailAccountId &accountId);
    void exportUpdates(const QMailAccountId &accountId);
    void synchronize(const QMailAccountId &accountId, uint minimum);

    void retrieveAll(const QMailAccountId &accountId);
    void synchronizeAllHelper(const QMailAccountId &accountId);
    void synchronizeAll(const QMailAccountId &accountId);
protected slots:
    void retrievalCompleted(quint64);

private:
    friend class QMailRetrievalAction;
};

class QMailExportUpdatesCommand : public QMailServiceActionCommand
{
public:
    QMailExportUpdatesCommand(QMailRetrievalActionPrivate *action, const QMailAccountId &accountId) :_action(action), _accountId(accountId) {};
    void execute() { _action->exportUpdatesHelper(_accountId); }
private:
    QMailRetrievalActionPrivate *_action;
    QMailAccountId _accountId;
};

class QMailSynchronizeCommand : public QMailServiceActionCommand
{
public:
    QMailSynchronizeCommand(QMailRetrievalActionPrivate *action, const QMailAccountId &accountId) :_action(action), _accountId(accountId) {};
    void execute() { _action->synchronizeAllHelper(_accountId); }
private:
    QMailRetrievalActionPrivate *_action;
    QMailAccountId _accountId;
};

class QMailRetrieveFolderListCommand : public QMailServiceActionCommand
{
public:
    QMailRetrieveFolderListCommand(QMailRetrievalActionPrivate *action, const QMailAccountId &accountId) :_action(action), _accountId(accountId) {};
    void execute() { _action->retrieveFolderListHelper(_accountId, QMailFolderId()); }
private:
    QMailRetrievalActionPrivate *_action;
    QMailAccountId _accountId;
};

class QMailRetrieveMessageListCommand : public QMailServiceActionCommand
{
public:
 QMailRetrieveMessageListCommand(QMailRetrievalActionPrivate *action, const QMailAccountId &accountId, uint minimum) 
     :_action(action), 
      _accountId(accountId),
      _minimum(minimum) {};
    void execute() { _action->retrieveMessageListHelper(_accountId, QMailFolderId(), _minimum, QMailMessageSortKey()); }
private:
    QMailRetrievalActionPrivate *_action;
    QMailAccountId _accountId;
    uint _minimum;
};

class QMailTransmitActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailTransmitActionPrivate(QMailTransmitAction *i);

    void transmitMessages(const QMailAccountId &accountId);
    void transmitMessage(const QMailMessageId &messageId);

signals:
    void messagesTransmitted(const QMailMessageIdList &ids);
    void messagesFailedTransmission(const QMailMessageIdList &ids, QMailServiceAction::Status::ErrorCode);

protected:
    virtual void init();

protected slots:
    void messagesTransmitted(quint64, const QMailMessageIdList &id);
    void messagesFailedTransmission(quint64, const QMailMessageIdList &id, QMailServiceAction::Status::ErrorCode);
    void transmissionCompleted(quint64);

private:
    friend class QMailTransmitAction;
};


class QMailStorageActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailStorageActionPrivate(QMailStorageAction *i);

    void onlineDeleteMessages(const QMailMessageIdList &ids);
    void onlineDeleteMessagesHelper(const QMailMessageIdList &ids);
    void discardMessages(const QMailMessageIdList &ids);

    void onlineCopyMessages(const QMailMessageIdList &ids, const QMailFolderId &destination);
    void onlineMoveMessages(const QMailMessageIdList &ids, const QMailFolderId &destination);
    void onlineFlagMessagesAndMoveToStandardFolder(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask);
    void addMessages(const QMailMessageList &list);
    void updateMessages(const QMailMessageList &list);
    void updateMessages(const QMailMessageMetaDataList &list);
    void deleteMessages(const QMailMessageIdList &ids);
    void rollBackUpdates(const QMailAccountId &mailAccountId);
    void moveToStandardFolder(const QMailMessageIdList& ids, quint64 standardFolder);
    void moveToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId);
    void flagMessages(const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask);
    void restoreToPreviousFolder(const QMailMessageKey& key);

    void onlineCreateFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    void onlineRenameFolder(const QMailFolderId &id, const QString &name);
    void onlineDeleteFolder(const QMailFolderId &id);
    void onlineDeleteFolderHelper(const QMailFolderId &id);
    void onlineMoveFolder(const QMailFolderId &id, const QMailFolderId &newParentId);

protected:
    virtual void init();

protected slots:
    void messagesEffected(quint64, const QMailMessageIdList &id);
    void storageActionCompleted(quint64);
    void messagesAdded(quint64, const QMailMessageIdList &ids);
    void messagesUpdated(quint64, const QMailMessageIdList &ids);

private:
    friend class QMailStorageAction;

    QMailMessageIdList _ids;
    QMailMessageIdList _addedOrUpdatedIds;
};

class QMailDeleteFolderCommand : public QMailServiceActionCommand
{
public:
    QMailDeleteFolderCommand(QMailStorageActionPrivate *action, const QMailFolderId &folderId) :_action(action), _folderId(folderId) {};
    void execute() { _action->onlineDeleteFolderHelper(_folderId); }
private:
    QMailStorageActionPrivate *_action;
    QMailFolderId _folderId;
};

class QMailMoveCommand : public QMailServiceActionCommand
{
public:
    QMailMoveCommand(QMailStorageActionPrivate *action, const QMailMessageIdList &ids, const QMailFolderId &destinationId) 
        :_action(action), _ids(ids), _folderId(destinationId) {};
    void execute() { _action->onlineMoveMessages(_ids, _folderId); }
private:
    QMailStorageActionPrivate *_action;
    QMailMessageIdList _ids;
    QMailFolderId _folderId;
};

class QMailDeleteMessagesCommand : public QMailServiceActionCommand
{
public:
    QMailDeleteMessagesCommand(QMailStorageActionPrivate *action, const QMailMessageIdList &ids) :_action(action), _ids(ids) {};
    void execute() { _action->onlineDeleteMessagesHelper(_ids); }
private:
    QMailStorageActionPrivate *_action;
    QMailMessageIdList _ids;
};

class QMailSearchActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailSearchActionPrivate(QMailSearchAction *i);
    virtual ~QMailSearchActionPrivate();

    void searchMessages(const QMailMessageKey &filter, const QString &bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);
    void searchMessages(const QMailMessageKey &filter, const QString &bodyText, QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort);
    void countMessages(const QMailMessageKey &filter, const QString &bodyText);
    void cancelOperation();

protected:
    virtual void init();

signals:
    void messageIdsMatched(const QMailMessageIdList &ids);
    void remainingMessagesCount(uint);
    void messagesCount(uint);

private slots:
    void matchingMessageIds(quint64 action, const QMailMessageIdList &ids);
    void remainingMessagesCount(quint64 action, uint count);
    void messagesCount(quint64 action, uint count);
    void searchCompleted(quint64 action);

    void finaliseSearch();

private:
    friend class QMailSearchAction;

    QMailMessageIdList _matchingIds;
    uint _remainingMessagesCount;
    uint _messagesCount;
};

class QMailActionInfoPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT
public:
    QMailActionInfoPrivate(const QMailActionData &data, QMailActionInfo *i);

    quint64 actionId() const;
    QMailServerRequestType requestType() const;
    float totalProgress() const;
    QMailActionInfo::Status::ErrorCode statusErrorCode() const;
    QString statusText() const;
    QMailAccountId statusAccountId() const;
    QMailFolderId statusFolderId() const;
    QMailMessageId statusMessageId() const;
signals:
    void statusErrorCodeChanged(QMailActionInfo::StatusErrorCode newError);
    void statusTextChanged(const QString &newText);
    void statusAccountIdChanged(const QMailAccountId &newAccountId);
    void statusFolderIdChanged(const QMailFolderId &newFolderId);
    void statusMessageIdChanged(const QMailMessageId &newMessageId);
    void totalProgressChanged(float progress);
public slots:
    void theStatusChanged(const QMailServiceAction::Status &newStatus);
    void theProgressChanged(uint progress, uint total);
private slots:
    void activityCompleted(quint64 action);

protected:
    QMailServiceAction::Status _lastStatus;
    QMailServerRequestType _requestType;
    bool _actionCompleted;
};

class QMailActionObserverPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT
public:
    QMailActionObserverPrivate(QMailActionObserver *i);
    void requestInitialization();
     QList< QSharedPointer<QMailActionInfo> > runningActions() const;
signals:
    void actionsChanged(const QList< QSharedPointer<QMailActionInfo> > &);
public slots:
    void listActionsRequest();
private slots:
    void anActionActivityChanged(QMailServiceAction::Activity activity);
    void removeOldActions();
    void actionsListed(const QMailActionDataList &actions);
    void actionStarted(const QMailActionData &action);
private:
    QSharedPointer<QMailActionInfo> addAction(const QMailActionData &action);
    QMap< QMailActionId, QSharedPointer<QMailActionInfo> > _runningActions;
    QList<QMailActionId> _delayRemoveList;
    bool _isReady;
};

class QMailProtocolActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailProtocolActionPrivate(QMailProtocolAction *i);

    void protocolRequest(const QMailAccountId &accountId, const QString &request, const QVariant &data);

signals:
    void protocolResponse(const QString &response, const QVariant &data);

private slots:
    void protocolResponse(quint64 action, const QString &response, const QVariant &data);
    void protocolRequestCompleted(quint64 action);

private:
    friend class QMailProtocolAction;
};

#endif
