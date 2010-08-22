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

    void deleteMessages(const QMailMessageIdList &ids);
    void deleteMessagesHelper(const QMailMessageIdList &ids);
    void discardMessages(const QMailMessageIdList &ids);

    void copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destination);
    void moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destination);
    void flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask);

    void createFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    void renameFolder(const QMailFolderId &id, const QString &name);
    void deleteFolder(const QMailFolderId &id);
    void deleteFolderHelper(const QMailFolderId &id);

protected:
    virtual void init();

protected slots:
    void messagesEffected(quint64, const QMailMessageIdList &id);
    void storageActionCompleted(quint64);

private:
    friend class QMailStorageAction;

    QMailMessageIdList _ids;
};

class QMailDeleteFolderCommand : public QMailServiceActionCommand
{
public:
    QMailDeleteFolderCommand(QMailStorageActionPrivate *action, const QMailFolderId &folderId) :_action(action), _folderId(folderId) {};
    void execute() { _action->deleteFolderHelper(_folderId); }
private:
    QMailStorageActionPrivate *_action;
    QMailFolderId _folderId;
};

class QMailMoveCommand : public QMailServiceActionCommand
{
public:
    QMailMoveCommand(QMailStorageActionPrivate *action, const QMailMessageIdList &ids, const QMailFolderId &destinationId) 
        :_action(action), _ids(ids), _folderId(destinationId) {};
    void execute() { _action->moveMessages(_ids, _folderId); }
private:
    QMailStorageActionPrivate *_action;
    QMailMessageIdList _ids;
    QMailFolderId _folderId;
};

class QMailDeleteMessagesCommand : public QMailServiceActionCommand
{
public:
    QMailDeleteMessagesCommand(QMailStorageActionPrivate *action, const QMailMessageIdList &ids) :_action(action), _ids(ids) {};
    void execute() { _action->deleteMessagesHelper(_ids); }
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
    void cancelOperation();

protected:
    virtual void init();

signals:
    void messageIdsMatched(const QMailMessageIdList &ids);

private slots:
    void matchingMessageIds(quint64 action, const QMailMessageIdList &ids);
    void searchCompleted(quint64 action);

    void finaliseSearch();

private:
    friend class QMailSearchAction;

    QMailMessageIdList _matchingIds;
};

class QMailActionInfoPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT
public:
    QMailActionInfoPrivate(QMailActionId action, QMailServerRequestType description, QMailActionInfo *i);

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
    void theStatusChanged(QMailServiceAction::Status newStatus);
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
