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

#include "qmailmessageserver.h"
#include <qcopadaptor_p.h>
#include <qcopchannel_p.h>
#include <qcopserver_p.h>

static bool connectIpc( QObject *sender, const QByteArray& signal,
        QObject *receiver, const QByteArray& member)
{
    return QCopAdaptor::connect(sender,signal,receiver,member);
}

class QMF_EXPORT QMailMessageServerPrivate : public QObject
{
    Q_OBJECT

    friend class QMailMessageServer;

public:
    QMailMessageServerPrivate(QMailMessageServer* parent);
    ~QMailMessageServerPrivate();

signals:
    void initialise();

    void transmitMessages(quint64, const QMailAccountId &accountId);
    void transmitMessage(quint64, const QMailMessageId &messageId);

    void retrieveFolderList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageLists(quint64, const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort);
    void retrieveMessageList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);
    void retrieveNewMessages(quint64, const QMailAccountId &accountId, const QMailFolderIdList &folderIds);

    void createStandardFolders(quint64, const QMailAccountId &accountId);

    void retrieveMessages(quint64, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(quint64, const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(quint64, const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(quint64, const QMailMessagePart::Location &partLocation, uint minimum);

    void retrieveAll(quint64, const QMailAccountId &accountId);
    void exportUpdates(quint64, const QMailAccountId &accountId);

    void synchronize(quint64, const QMailAccountId &accountId);

    void onlineCopyMessages(quint64, const QMailMessageIdList& mailList, const QMailFolderId &destination);
    void onlineMoveMessages(quint64, const QMailMessageIdList& mailList, const QMailFolderId &destination);
    void onlineFlagMessagesAndMoveToStandardFolder(quint64, const QMailMessageIdList& mailList, quint64 setMask, quint64 unsetMask);
    void addMessages(quint64, const QString &filename);
    void addMessages(quint64, const QMailMessageMetaDataList &list);
    void updateMessages(quint64, const QString &filename);
    void updateMessages(quint64, const QMailMessageMetaDataList &list);
    void deleteMessages(quint64, const QMailMessageIdList &ids);
    void rollBackUpdates(quint64, const QMailAccountId &mailAccountId);
    void moveToStandardFolder(quint64, const QMailMessageIdList& ids, quint64 standardFolder);
    void moveToFolder(quint64, const QMailMessageIdList& ids, const QMailFolderId& folderId);
    void flagMessages(quint64, const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask);
    void restoreToPreviousFolder(quint64, const QMailMessageKey& key);
    void onlineCreateFolder(quint64, const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    void onlineRenameFolder(quint64, const QMailFolderId &folderId, const QString &name);
    void onlineDeleteFolder(quint64, const QMailFolderId &folderId);
    void onlineMoveFolder(quint64, const QMailFolderId &folderId, const QMailFolderId &newParentId);

    void cancelTransfer(quint64);

    void onlineDeleteMessages(quint64, const QMailMessageIdList& id, QMailStore::MessageRemovalOption);

    void searchMessages(quint64, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);
    void searchMessages(quint64, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort);
    void countMessages(quint64, const QMailMessageKey& filter, const QString& bodyText);

    void cancelSearch(quint64);

    void shutdown();

    void listActions();

    void protocolRequest(quint64, const QMailAccountId &accountId, const QString &request, const QVariant &data);

    void acknowledgeNewMessages(const QMailMessageTypeList&);

private:
    QCopAdaptor* adaptor;
};


QMailMessageServerPrivate::QMailMessageServerPrivate(QMailMessageServer* parent)
    : QObject(parent),
      adaptor(new QCopAdaptor(QLatin1String("QPE/QMailMessageServer"), this))
{
    // Forward signals to the message server
    connectIpc(adaptor, MESSAGE(newCountChanged(QMailMessageCountMap)),
               parent, SIGNAL(newCountChanged(QMailMessageCountMap)));
    connectIpc(this, SIGNAL(acknowledgeNewMessages(QMailMessageTypeList)),
               adaptor, MESSAGE(acknowledgeNewMessages(QMailMessageTypeList)));

    connectIpc(this, SIGNAL(initialise()),
               adaptor, MESSAGE(initialise()));
    connectIpc(this, SIGNAL(transmitMessages(quint64, QMailAccountId)),
               adaptor, MESSAGE(transmitMessages(quint64, QMailAccountId)));
    connectIpc(this, SIGNAL(transmitMessage(quint64, QMailMessageId)),
               adaptor, MESSAGE(transmitMessage(quint64, QMailMessageId)));
    connectIpc(this, SIGNAL(retrieveFolderList(quint64, QMailAccountId, QMailFolderId, bool)),
               adaptor, MESSAGE(retrieveFolderList(quint64, QMailAccountId, QMailFolderId, bool)));
    connectIpc(this, SIGNAL(retrieveMessageList(quint64, QMailAccountId, QMailFolderId, uint, QMailMessageSortKey)),
               adaptor, MESSAGE(retrieveMessageList(quint64, QMailAccountId, QMailFolderId, uint, QMailMessageSortKey)));
    connectIpc(this, SIGNAL(retrieveMessageLists(quint64, QMailAccountId, QMailFolderIdList, uint, QMailMessageSortKey)),
               adaptor, MESSAGE(retrieveMessageLists(quint64, QMailAccountId, QMailFolderIdList, uint, QMailMessageSortKey)));
    connectIpc(this, SIGNAL(retrieveNewMessages(quint64, QMailAccountId, QMailFolderIdList)),
               adaptor, MESSAGE(retrieveNewMessages(quint64, QMailAccountId, QMailFolderIdList)));
    connectIpc(this, SIGNAL(createStandardFolders(quint64, QMailAccountId)),
               adaptor, MESSAGE(createStandardFolders(quint64, QMailAccountId)));
    connectIpc(this, SIGNAL(retrieveMessages(quint64, QMailMessageIdList, QMailRetrievalAction::RetrievalSpecification)),
               adaptor, MESSAGE(retrieveMessages(quint64, QMailMessageIdList, QMailRetrievalAction::RetrievalSpecification)));
    connectIpc(this, SIGNAL(retrieveMessagePart(quint64, QMailMessagePart::Location)),
               adaptor, MESSAGE(retrieveMessagePart(quint64, QMailMessagePart::Location)));
    connectIpc(this, SIGNAL(retrieveMessageRange(quint64, QMailMessageId, uint)),
               adaptor, MESSAGE(retrieveMessageRange(quint64, QMailMessageId, uint)));
    connectIpc(this, SIGNAL(retrieveMessagePartRange(quint64, QMailMessagePart::Location, uint)),
               adaptor, MESSAGE(retrieveMessagePartRange(quint64, QMailMessagePart::Location, uint)));
    connectIpc(this, SIGNAL(retrieveAll(quint64, QMailAccountId)),
               adaptor, MESSAGE(retrieveAll(quint64, QMailAccountId)));
    connectIpc(this, SIGNAL(exportUpdates(quint64, QMailAccountId)),
               adaptor, MESSAGE(exportUpdates(quint64, QMailAccountId)));
    connectIpc(this, SIGNAL(synchronize(quint64, QMailAccountId)),
               adaptor, MESSAGE(synchronize(quint64, QMailAccountId)));
    connectIpc(this, SIGNAL(cancelTransfer(quint64)),
               adaptor, MESSAGE(cancelTransfer(quint64)));
    connectIpc(this, SIGNAL(onlineCopyMessages(quint64, QMailMessageIdList, QMailFolderId)),
               adaptor, MESSAGE(onlineCopyMessages(quint64, QMailMessageIdList, QMailFolderId)));
    connectIpc(this, SIGNAL(onlineMoveMessages(quint64, QMailMessageIdList, QMailFolderId)),
               adaptor, MESSAGE(onlineMoveMessages(quint64, QMailMessageIdList, QMailFolderId)));
    connectIpc(this, SIGNAL(onlineDeleteMessages(quint64, QMailMessageIdList, QMailStore::MessageRemovalOption)),
               adaptor, MESSAGE(onlineDeleteMessages(quint64, QMailMessageIdList, QMailStore::MessageRemovalOption)));
    connectIpc(this, SIGNAL(onlineFlagMessagesAndMoveToStandardFolder(quint64, QMailMessageIdList, quint64, quint64)),
               adaptor, MESSAGE(onlineFlagMessagesAndMoveToStandardFolder(quint64, QMailMessageIdList, quint64, quint64)));
    connectIpc(this, SIGNAL(addMessages(quint64, QString)),
               adaptor, MESSAGE(addMessages(quint64, QString)));
    connectIpc(this, SIGNAL(addMessages(quint64, QMailMessageMetaDataList)),
               adaptor, MESSAGE(addMessages(quint64, QMailMessageMetaDataList)));
    connectIpc(this, SIGNAL(updateMessages(quint64, QString)),
               adaptor, MESSAGE(updateMessages(quint64, QString)));
    connectIpc(this, SIGNAL(updateMessages(quint64, QMailMessageMetaDataList)),
               adaptor, MESSAGE(updateMessages(quint64, QMailMessageMetaDataList)));
    connectIpc(this, SIGNAL(onlineCreateFolder(quint64, QString, QMailAccountId, QMailFolderId)),
               adaptor, MESSAGE(onlineCreateFolder(quint64, QString, QMailAccountId, QMailFolderId)));
    connectIpc(this, SIGNAL(onlineRenameFolder(quint64, QMailFolderId, QString)),
               adaptor, MESSAGE(onlineRenameFolder(quint64, QMailFolderId, QString)));
    connectIpc(this, SIGNAL(onlineMoveFolder(quint64, QMailFolderId, QMailFolderId)),
               adaptor, MESSAGE(onlineMoveFolder(quint64, QMailFolderId, QMailFolderId)));
    connectIpc(this, SIGNAL(onlineDeleteFolder(quint64, QMailFolderId)),
               adaptor, MESSAGE(onlineDeleteFolder(quint64, QMailFolderId)));
    connectIpc(this, SIGNAL(deleteMessages(quint64, QMailMessageIdList)),
               adaptor, MESSAGE(deleteMessages(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(rollBackUpdates(quint64, QMailAccountId)),
               adaptor, MESSAGE(rollBackUpdates(quint64, QMailAccountId)));
    connectIpc(this, SIGNAL(moveToStandardFolder(quint64, QMailMessageIdList, quint64)),
               adaptor, MESSAGE(moveToStandardFolder(quint64, QMailMessageIdList, quint64)));
    connectIpc(this, SIGNAL(moveToFolder(quint64, QMailMessageIdList, QMailFolderId)),
               adaptor, MESSAGE(moveToFolder(quint64, QMailMessageIdList, QMailFolderId)));
    connectIpc(this, SIGNAL(flagMessages(quint64, QMailMessageIdList, quint64, quint64)),
               adaptor, MESSAGE(flagMessages(quint64, QMailMessageIdList, quint64, quint64)));
    connectIpc(this, SIGNAL(restoreToPreviousFolder(quint64, QMailMessageKey)),
               adaptor, MESSAGE(restoreToPreviousFolder(quint64, QMailMessageKey)));
    connectIpc(this, SIGNAL(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, QMailMessageSortKey)),
               adaptor, MESSAGE(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, QMailMessageSortKey)));
    connectIpc(this, SIGNAL(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, quint64, QMailMessageSortKey)),
               adaptor, MESSAGE(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, quint64, QMailMessageSortKey)));
    connectIpc(this, SIGNAL(countMessages(quint64, QMailMessageKey, QString)),
               adaptor, MESSAGE(countMessages(quint64, QMailMessageKey, QString)));
    connectIpc(this, SIGNAL(cancelSearch(quint64)),
               adaptor, MESSAGE(cancelSearch(quint64)));
    connectIpc(this, SIGNAL(shutdown()),
               adaptor, MESSAGE(shutdown()));
    connectIpc(this, SIGNAL(listActions()),
               adaptor, MESSAGE(listActions()));
    connectIpc(this, SIGNAL(protocolRequest(quint64, QMailAccountId, QString, QVariant)),
               adaptor, MESSAGE(protocolRequest(quint64, QMailAccountId, QString, QVariant)));

    // Propagate received events as exposed signals
    connectIpc(adaptor, MESSAGE(actionStarted(QMailActionData)),
               parent, SIGNAL(actionStarted(QMailActionData)));
    connectIpc(adaptor, MESSAGE(activityChanged(quint64, QMailServiceAction::Activity)),
               parent, SIGNAL(activityChanged(quint64, QMailServiceAction::Activity)));
    connectIpc(adaptor, MESSAGE(connectivityChanged(quint64, QMailServiceAction::Connectivity)),
               parent, SIGNAL(connectivityChanged(quint64, QMailServiceAction::Connectivity)));
    connectIpc(adaptor, MESSAGE(statusChanged(quint64, const QMailServiceAction::Status)),
               parent, SIGNAL(statusChanged(quint64, const QMailServiceAction::Status)));
    connectIpc(adaptor, MESSAGE(progressChanged(quint64, uint, uint)),
               parent, SIGNAL(progressChanged(quint64, uint, uint)));
    connectIpc(adaptor, MESSAGE(messagesDeleted(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesDeleted(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(messagesCopied(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesCopied(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(messagesMoved(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesMoved(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(messagesFlagged(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesFlagged(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(messagesAdded(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesAdded(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(messagesUpdated(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesUpdated(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(folderCreated(quint64, QMailFolderId)),
               parent, SIGNAL(folderCreated(quint64, QMailFolderId)));
    connectIpc(adaptor, MESSAGE(folderRenamed(quint64, QMailFolderId)),
               parent, SIGNAL(folderRenamed(quint64, QMailFolderId)));
    connectIpc(adaptor, MESSAGE(folderDeleted(quint64, QMailFolderId)),
               parent, SIGNAL(folderDeleted(quint64, QMailFolderId)));
    connectIpc(adaptor, MESSAGE(folderMoved(quint64, QMailFolderId)),
               parent, SIGNAL(folderMoved(quint64, QMailFolderId)));
    connectIpc(adaptor, MESSAGE(storageActionCompleted(quint64)),
               parent, SIGNAL(storageActionCompleted(quint64)));
    connectIpc(adaptor, MESSAGE(retrievalCompleted(quint64)),
               parent, SIGNAL(retrievalCompleted(quint64)));
    connectIpc(adaptor, MESSAGE(messagesTransmitted(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesTransmitted(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(messagesFailedTransmission(quint64, QMailMessageIdList, QMailServiceAction::Status::ErrorCode)),
               parent, SIGNAL(messagesFailedTransmission(quint64, QMailMessageIdList, QMailServiceAction::Status::ErrorCode)));
    connectIpc(adaptor, MESSAGE(transmissionCompleted(quint64)),
               parent, SIGNAL(transmissionCompleted(quint64)));
    connectIpc(adaptor, MESSAGE(matchingMessageIds(quint64, QMailMessageIdList)),
               parent, SIGNAL(matchingMessageIds(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(remainingMessagesCount(quint64, uint)),
               parent, SIGNAL(remainingMessagesCount(quint64, uint)));
    connectIpc(adaptor, MESSAGE(messagesCount(quint64, uint)),
               parent, SIGNAL(messagesCount(quint64, uint)));
    connectIpc(adaptor, MESSAGE(searchCompleted(quint64)),
               parent, SIGNAL(searchCompleted(quint64)));
    connectIpc(adaptor, MESSAGE(actionsListed(QMailActionDataList)),
               parent, SIGNAL(actionsListed(QMailActionDataList)));
    connectIpc(adaptor, MESSAGE(protocolResponse(quint64, QString, QVariant)),
               parent, SIGNAL(protocolResponse(quint64, QString, QVariant)));
    connectIpc(adaptor, MESSAGE(protocolRequestCompleted(quint64)),
               parent, SIGNAL(protocolRequestCompleted(quint64)));
    QObject::connect(adaptor, SIGNAL(connectionDown()),
                     parent, SIGNAL(connectionDown()));
    QObject::connect(adaptor, SIGNAL(reconnectionTimeout()),
                     parent, SIGNAL(reconnectionTimeout()));
}

QMailMessageServerPrivate::~QMailMessageServerPrivate()
{
}


/*!
    \class QMailMessageServer

    \preliminary
    \brief The QMailMessageServer class provides signals and slots which implement a convenient
    interface for communicating with the MessageServer process via IPC.

    \ingroup messaginglibrary

    QMF client messaging applications can send and receive messages of various types by
    communicating with the MessageServer.  The MessageServer
    is a separate process, communicating with clients via inter-process messages.  
    QMailMessageServer acts as a proxy object for the server process, providing an
    interface for communicating with the MessageServer by the use of signals and slots 
    in the client process.  It provides Qt signals corresponding to messages received from 
    the MessageServer application, and Qt slots which send messages to the MessageServer 
    when invoked.

    For most messaging client applications, the QMailServiceAction objects offer a simpler
    interface for requesting actions from the messageserver, and assessing their results.

    \section1 Sending Messages

    To send messages, the client should construct instances of the QMailMessage class
    formulated to contain the desired content.  These messages should be stored to the
    mail store, within the Outbox folder configured for the parent account.

    An instance of QMailTransmitAction should be used to request transmission of the 
    outgoing messages.

    \section1 Retrieving Messages

    There are a variety of mechanisms for retrieving messages, at various levels of
    granularity.  In all cases, retrieved messages are added directly to the mail 
    store by the message server, from where clients can retrieve their meta data or
    content.

    An instance of QMailRetrievalAction should be used to request retrieval of 
    folders and messages.

    \sa QMailServiceAction, QMailStore
*/

/*!
    \fn void QMailMessageServer::activityChanged(quint64 action, QMailServiceAction::Activity activity);

    Emitted whenever the MessageServer experiences a change in the activity status of the request
    identified by \a action.  The request's new status is described by \a activity.
*/

/*!
    \fn void QMailMessageServer::connectivityChanged(quint64 action, QMailServiceAction::Connectivity connectivity);

    Emitted whenever the MessageServer has a change in connectivity while servicing the request 
    identified by \a action.  The new server connectivity status is described by \a connectivity.
*/

/*!
    \fn void QMailMessageServer::statusChanged(quint64 action, const QMailServiceAction::Status status);

    Emitted whenever the MessageServer experiences a status change that may be of interest to the client,
    while servicing the request identified by \a action.  The new server status is described by \a status.
*/

/*!
    \fn void QMailMessageServer::progressChanged(quint64 action, uint progress, uint total);

    Emitted when the progress of the request identified by \a action changes; 
    \a total indicates the extent of the operation to be performed, \a progress indicates the current degree of completion.
*/

/*!
    \fn void QMailMessageServer::newCountChanged(const QMailMessageCountMap& counts);

    Emitted when the count of 'new' messages changes; the new count is described by \a counts.
    
    \deprecated

    \sa acknowledgeNewMessages()
*/

/*!
    \fn void QMailMessageServer::retrievalCompleted(quint64 action);

    Emitted when the retrieval operation identified by \a action is completed.
*/

/*!
    \fn void QMailMessageServer::messagesTransmitted(quint64 action, const QMailMessageIdList& list);

    Emitted when the messages identified by \a list have been transmitted to the external server,
    in response to the request identified by \a action.

    \sa transmitMessages()
*/

/*!
    \fn void QMailMessageServer::messagesFailedTransmission(quint64 action, const QMailMessageIdList& list, QMailServiceAction::Status::ErrorCode error);

    Emitted when a failed attempt has been made to transmit messages identified by \a list to the external server,
    in response to the request identified by \a action.
    
    The error is described by \a error.

    \sa transmitMessages()
*/

/*!
    \fn void QMailMessageServer::transmissionCompleted(quint64 action);

    Emitted when the transmit operation identified by \a action is completed.

    \sa transmitMessages()
*/

/*!
    \fn void QMailMessageServer::messagesDeleted(quint64 action, const QMailMessageIdList& list);

    Emitted when the messages identified by \a list have been deleted from the mail store,
    in response to the request identified by \a action.

    \sa deleteMessages(), onlineDeleteMessages()
*/

/*!
    \fn void QMailMessageServer::messagesCopied(quint64 action, const QMailMessageIdList& list);

    Emitted when the messages identified by \a list have been copied to the destination 
    folder on the external service, in response to the request identified by \a action.

    \sa onlineCopyMessages()
*/

/*!
    \fn void QMailMessageServer::messagesMoved(quint64 action, const QMailMessageIdList& list);

    Emitted when the messages identified by \a list have been moved to the destination 
    folder on the external service, in response to the request identified by \a action.

    \sa moveToFolder(), moveToStandardFolder(), onlineMoveMessages()
*/

/*!
    \fn void QMailMessageServer::messagesFlagged(quint64 action, const QMailMessageIdList& list);

    Emitted when the messages identified by \a list have been flagged with the specified
    set of status flags, in response to the request identified by \a action.

    \sa flagMessages(), moveToStandardFolder(), onlineFlagMessagesAndMoveToStandardFolder()
*/

/*!
    \fn void QMailMessageServer::folderCreated(quint64 action, const QMailFolderId& folderId);

    Emitted when the folder identified by \a folderId has been created, in response to the request
    identified by \a action.

    \sa onlineCreateFolder()
*/

/*!
    \fn void QMailMessageServer::folderRenamed(quint64 action, const QMailFolderId& folderId);

    Emitted when the folder identified by \a folderId has been renamed, in response to the request
    identified by \a action.

    \sa onlineRenameFolder()
*/

/*!
    \fn void QMailMessageServer::folderDeleted(quint64 action, const QMailFolderId& folderId);

    Emitted when the folder identified by \a folderId has been deleted, in response to the request
    identified by \a action.

    \sa onlineDeleteFolder()
*/

/*!
    \fn void QMailMessageServer::folderMoved(quint64 action, const QMailFolderId& folderId);

    Emitted when the folder identified by \a folderId has been moved, in response to the request
    identified by \a action.

    \sa onlineMoveFolder()
*/

/*!
    \fn void QMailMessageServer::storageActionCompleted(quint64 action);

    Emitted when the storage operation identified by \a action is completed.
*/

/*!
    \fn void QMailMessageServer::searchCompleted(quint64 action);

    Emitted when the search operation identified by \a action is completed.

    \sa searchMessages()
*/

/*!
    \fn void QMailMessageServer::matchingMessageIds(quint64 action, const QMailMessageIdList& ids);

    Emitted by the search operation identified by \a action; 
    \a ids contains the list of message identifiers located by the search.

    \sa searchMessages()
*/

/*!
    \fn void QMailMessageServer::remainingMessagesCount(quint64 action, uint count);

    Emitted by search operation identified by \a action; 
    Returns the \a count of matching messages remaining on the remote server, that is the count
    of messages that will not be retrieved from the remote server to the device.

    Only applicable for remote searches.

    \sa searchMessages()
*/

/*!
    \fn void QMailMessageServer::messagesCount(quint64 action, uint count);

    Emitted by search operation identified by \a action; 
    Returns the \a count of matching messages on the remote server.

    Only applicable for remote searches.

    \sa countMessages()
*/

/*!
    \fn void QMailMessageServer::protocolResponse(quint64 action, const QString &response, const QVariant &data);

    Emitted when the protocol request identified by \a action generates the response
    \a response, with the associated \a data.

    \sa protocolRequest()
*/

/*!
    \fn void QMailMessageServer::protocolRequestCompleted(quint64 action);

    Emitted when the protocol request identified by \a action is completed.

    \sa protocolRequest()
*/

/*!
    \fn void QMailMessageServer::actionsListed(const QMailActionDataList &list);

    Emitted when a list of running actions has been retrieved from the server. 
    The list of running actions is described by \a list.
*/

/*!
    \fn void QMailMessageServer::actionStarted(QMailActionData data)

    Emitted when the action described by \a data has been started on the 
    messageserver.
*/

/*!
    Constructs a QMailMessageServer object with parent \a parent, and initiates communication with the MessageServer application.
*/
QMailMessageServer::QMailMessageServer(QObject* parent)
    : QObject(parent),
      d(new QMailMessageServerPrivate(this))
{
}

/*!
    Destroys the QMailMessageServer object.
*/
QMailMessageServer::~QMailMessageServer()
{
}

/*!
    Requests that the MessageServer application transmit any messages belonging to the
    account identified by \a accountId that are currently in the Outbox folder.
    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.
    
    \sa transmissionCompleted()
*/
void QMailMessageServer::transmitMessages(quint64 action, const QMailAccountId &accountId)
{
    emit d->transmitMessages(action, accountId);
}

/*!
    Requests that the MessageServer application transmit the message
    identified by \a messageId that are currently in the Outbox folder.
    The request has the identifier \a action.

    \sa transmissionCompleted()
*/
void QMailMessageServer::transmitMessage(quint64 action, const QMailMessageId &messageId)
{
    emit d->transmitMessage(action, messageId);
}

/*!
    Requests that the message server retrieve the list of folders available for the account \a accountId.
    If \a folderId is valid, the folders within that folder should be retrieved.  If \a descending is true,
    the search should also recursively retrieve the folders available within the previously retrieved folders.
    The request has the identifier \a action.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveFolderList(quint64 action, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    emit d->retrieveFolderList(action, accountId, folderId, descending);
}

/*!
    Requests that the message server retrieve the list of messages available for the account \a accountId.
    If \a folderId is valid, then only messages within that folder should be retrieved; otherwise 
    messages within all folders in the account should be retrieved. If a folder messages are being 
    retrieved from contains at least \a minimum messages then the messageserver should ensure that at 
    least \a minimum messages are available from the mail store for that folder; otherwise if the
    folder contains less than \a minimum messages the messageserver should ensure all the messages for 
    that folder are available from the mail store.
    
    If \a sort is not empty, the external service will 
    discover the listed messages in the ordering indicated by the sort criterion, if possible.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessageList(quint64 action, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    emit d->retrieveMessageList(action, accountId, folderId, minimum, sort);
}

/*!
    Requests that the messageserver retrieve the list of messages available for the account \a accountId.
    If \a folderIds is not empty, then only messages within those folders should be retrieved; otherwise 
    no messages should be retrieved. If a folder messages are being 
    retrieved from contains at least \a minimum messages then the messageserver should ensure that at 
    least \a minimum messages are available from the mail store for that folder; otherwise if the
    folder contains less than \a minimum messages the messageserver should ensure all the messages for 
    that folder are available from the mail store.
    
    If \a sort is not empty, the external service will 
    discover the listed messages in the ordering indicated by the sort criterion, if possible.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessageLists(quint64 action, const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort)
{
    emit d->retrieveMessageLists(action, accountId, folderIds, minimum, sort);
}

/*!
    Requests that the message server retrieve new messages for the account \a accountId in the
    folders specified by \a folderIds.
    
    If a folder inspected has been previously inspected then new mails in that folder will
    be retrieved, otherwise the most recent message in that folder, if any, will be retrieved.
    
    This function is intended for use by protocol plugins to retrieve new messages when
    a push notification event is received from the remote server.
    
    Detection of deleted messages, and flag updates for messages in the mail store will
    not necessarily be performed.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveNewMessages(quint64 action, const QMailAccountId &accountId, const QMailFolderIdList &folderIds)
{
    emit d->retrieveNewMessages(action, accountId, folderIds);
}

/*!
    Requests that the message server create the standard folders for the
    account \a accountId. If all standard folders are already set in the storage
    the service action will return success immediately, in case some standard folders are
    not set, a matching attempt against a predefined list of translations will be made,
    if the folders can't be matched, messageserver will try to create them in the server side
    and match them if the creation is successful. In case folder creation is not allowed for
    the account \a accountId the service action will still complete successfully, clients
    must use local standard folders only in this case.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrieveFolderList
*/
void QMailMessageServer::createStandardFolders(quint64 action, const QMailAccountId &accountId)
{
    emit d->createStandardFolders(action, accountId);
}

/*!
    Requests that the message server retrieve data regarding the messages identified by \a messageIds.  

    If \a spec is \l QMailRetrievalAction::Flags, then the message server should detect if 
    the read or important status of messages identified by \a messageIds has changed on the server 
    or if the messages have been removed on the server.
    The \l QMailMessage::ReadElsewhere, \l QMailMessage::ImportantElsewhere and \l QMailMessage::Removed 
    status flags of messages will be updated to reflect the status of the message on the server.

    If \a spec is \l QMailRetrievalAction::MetaData, then the message server should 
    retrieve the meta data of the each message listed in \a messageIds.
    
    If \a spec is \l QMailRetrievalAction::Content, then the message server should 
    retrieve the entirety of each message listed in \a messageIds.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessages(quint64 action, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec)
{
    emit d->retrieveMessages(action, messageIds, spec);
}

/*!
    Requests that the message server retrieve the message part that is indicated by the 
    location \a partLocation.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessagePart(quint64 action, const QMailMessagePart::Location &partLocation)
{
    emit d->retrieveMessagePart(action, partLocation);
}

/*!
    Requests that the message server retrieve a subset of the message \a messageId, such that
    at least \a minimum bytes are available from the mail store.
    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessageRange(quint64 action, const QMailMessageId &messageId, uint minimum)
{
    emit d->retrieveMessageRange(action, messageId, minimum);
}

/*!
    Requests that the message server retrieve a subset of the message part that is indicated by 
    the location \a partLocation.  The messageserver should ensure that at least \a minimum 
    bytes are available from the mail store.
    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessagePartRange(quint64 action, const QMailMessagePart::Location &partLocation, uint minimum)
{
    emit d->retrieveMessagePartRange(action, partLocation, minimum);
}

/*!
    Requests that the message server retrieve the meta data for all messages available 
    for the account \a accountId.
    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveAll(quint64 action, const QMailAccountId &accountId)
{
    emit d->retrieveAll(action, accountId);
}

/*!
    Requests that the message server update the external server with changes that have 
    been effected on the local device for account \a accountId.
    Local changes to \l QMailMessage::Read, and \l QMailMessage::Important message status 
    flags should be exported to the external server, and messages that have been removed
    using the \l QMailStore::CreateRemovalRecord option should be removed from the 
    external server.
    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::exportUpdates(quint64 action, const QMailAccountId &accountId)
{
    emit d->exportUpdates(action, accountId);
}

/*!
    Requests that the message server synchronize the messages and folders in the account 
    identified by \a accountId.

    Newly discovered messages should have their meta data retrieved,
    local changes to \l QMailMessage::Read, and \l QMailMessage::Important message status 
    flags should be exported to the external server, and messages that have been removed
    locally using the \l QMailStore::CreateRemovalRecord option should be removed from the 
    external server.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa retrievalCompleted()
*/
void QMailMessageServer::synchronize(quint64 action, const QMailAccountId &accountId)
{
    emit d->synchronize(action, accountId);
}

/*!
    Requests that the MessageServer create a copy of each message listed in \a mailList 
    in the folder identified by \a destinationId.
    
    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.
*/
void QMailMessageServer::onlineCopyMessages(quint64 action, const QMailMessageIdList& mailList, const QMailFolderId &destinationId)
{
    emit d->onlineCopyMessages(action, mailList, destinationId);
}

/*!
    Requests that the MessageServer move each message listed in \a mailList from its 
    current location to the folder identified by \a destinationId.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.
*/
void QMailMessageServer::onlineMoveMessages(quint64 action, const QMailMessageIdList& mailList, const QMailFolderId &destinationId)
{
    emit d->onlineMoveMessages(action, mailList, destinationId);
}

/*!
    Requests that the MessageServer flag each message listed in \a mailList by setting
    the status flags set in \a setMask, and unsetting the status flags set in \a unsetMask.
    The request has the identifier \a action.

    The protocol must ensure that the local message records are appropriately modified, 
    although the external changes may be buffered and effected at the next invocation 
    of exportUpdates().

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.
*/
void QMailMessageServer::onlineFlagMessagesAndMoveToStandardFolder(quint64 action, const QMailMessageIdList& mailList, quint64 setMask, quint64 unsetMask)
{
    emit d->onlineFlagMessagesAndMoveToStandardFolder(action, mailList, setMask, unsetMask);
}

/*!
    Requests that the MessageServer add the messages in 
    \a filename to the message store.

    The request has the identifier \a action.

    \deprecated
*/
void QMailMessageServer::addMessages(quint64 action, const QString& filename)
{
    emit d->addMessages(action, filename);
}

/*!
    Requests that the MessageServer update the list of \a messages
    in the message store, and ensure the durability of the content of \a messages.

    The request has the identifier \a action.
*/
void QMailMessageServer::addMessages(quint64 action, const QMailMessageMetaDataList& messages)
{
    emit d->addMessages(action, messages);
}

/*!
    Requests that the MessageServer update the messages in 
    \a filename to the message store.

    The request has the identifier \a action.

    \deprecated
*/
void QMailMessageServer::updateMessages(quint64 action, const QString& filename)
{
    emit d->updateMessages(action, filename);
}

/*!
    Requests that the MessageServer add the list of \a messages
    to the message store, and ensure the durability of the content of \a messages.

    The request has the identifier \a action.
*/
void QMailMessageServer::updateMessages(quint64 action, const QMailMessageMetaDataList& messages)
{
    emit d->updateMessages(action, messages);
}



/*!
    Requests that the MessageServer create a new folder named \a name, created in the
    account identified by \a accountId.

    If \a parentId is a valid folder identifier the new folder will be a child of the parent;
    otherwise the folder will be have no parent and will be created at the highest level.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa onlineDeleteFolder()
*/
void QMailMessageServer::onlineCreateFolder(quint64 action, const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId)
{
    emit d->onlineCreateFolder(action, name, accountId, parentId);
}

/*!
    Requests that the MessageServer rename the folder identified by \a folderId to \a name.
    The request has the identifier \a action.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa onlineCreateFolder()
*/
void QMailMessageServer::onlineRenameFolder(quint64 action, const QMailFolderId &folderId, const QString &name)
{
    emit d->onlineRenameFolder(action, folderId, name);
}

/*!
    Requests that the MessageServer delete the folder identified by \a folderId.
    Any existing folders or messages contained by the folder will also be deleted.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa onlineCreateFolder(), onlineRenameFolder(), onlineMoveFolder()
*/
void QMailMessageServer::onlineDeleteFolder(quint64 action, const QMailFolderId &folderId)
{
    emit d->onlineDeleteFolder(action, folderId);
}

/*!
    Requests that the MessageServer move the folder identified by \a folderId.
    If \a parentId is a valid folder identifier the new folder will be a child of the parent;
    otherwise the folder will be have no parent and will be created at the highest level.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication
    with external servers.

    \sa onlineCreateFolder(), onlineRenameFolder()
*/
void QMailMessageServer::onlineMoveFolder(quint64 action, const QMailFolderId &folderId, const QMailFolderId &newParentId)
{
    emit d->onlineMoveFolder(action, folderId, newParentId);
}

/*!
    Requests that the MessageServer cancel any pending transfer operations for the request identified by \a action.

    \sa transmitMessages(), retrieveMessages()
*/
void QMailMessageServer::cancelTransfer(quint64 action)
{
    emit d->cancelTransfer(action);
}

/*!
    Requests that the MessageServer reset the counts of 'new' messages to zero, for
    each message type listed in \a types.
    
    \deprecated

    \sa newCountChanged()
*/
void QMailMessageServer::acknowledgeNewMessages(const QMailMessageTypeList& types)
{
    emit d->acknowledgeNewMessages(types);
}

/*!
    Requests that the MessageServer delete the messages in \a mailList from the external
    server, if necessary for the relevant message type.  
    
    If \a option is 
    \l{QMailStore::CreateRemovalRecord}{CreateRemovalRecord} then a QMailMessageRemovalRecord
    will be created in the mail store for each deleted message. In this case
    the function requires the device to be online, it may initiate communication 
    with external servers.
    
    The request has the identifier \a action.

    \sa QMailStore::removeMessage()
*/
void QMailMessageServer::onlineDeleteMessages(quint64 action, const QMailMessageIdList& mailList, QMailStore::MessageRemovalOption option)
{
    emit d->onlineDeleteMessages(action, mailList, option);
}

/*!
    Requests that the MessageServer delete the messages in \a mailList, messages
    will be removed locally from the device, and if necessary information needed 
    to delete messages from an external server is recorded.

    Deleting messages using this slot does not initiate communication with any external
    server; Deletion from the external server will occur when 
    QMailRetrievalAction::exportUpdates is called successfully.
    
    The request has the identifier \a action.

    \sa QMailStore::removeMessage()
*/
void QMailMessageServer::deleteMessages(quint64 action, const QMailMessageIdList& mailList)
{
    emit d->deleteMessages(action, mailList);
}

/*!
    Asynchronous version of QMailDisconnected::rollBackUpdates()
    
    Rolls back all disconnected move and copy operations that have been applied to the 
    message store since the most recent synchronization of the message with the account 
    specified by \a mailAccountId.
    
    The request has the identifier \a action.
    
    \sa QMailDisconnected::updatesOutstanding()
*/
void QMailMessageServer::rollBackUpdates(quint64 action, const QMailAccountId &mailAccountId)
{
    emit d->rollBackUpdates(action, mailAccountId);
}

/*!
    Asynchronous version of QMailDisconnected::moveToStandardFolder()

    Disconnected moves the list of messages identified by \a ids into the standard folder \a standardFolder, setting standard 
    folder flags as appropriate.
    
    The move operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
            
    The request has the identifier \a action.

    \sa QMailDisconnected::moveToStandardFolder()
*/
void QMailMessageServer::moveToStandardFolder(quint64 action, const QMailMessageIdList& ids, quint64 standardFolder)
{
    emit d->moveToStandardFolder(action, ids, standardFolder);
}

/*!
    Asynchronous version of QMailDisconnected::moveToFolder()

    Disconnected moves the list of messages identified by \a ids into the folder identified by \a folderId, setting standard 
    folder flags as appropriate.

    Moving to another account is not supported.

    The move operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
    
    The request has the identifier \a action.
    
    \sa QMailDisconnected::moveToFolder()
*/
void QMailMessageServer::moveToFolder(quint64 action, const QMailMessageIdList& ids, const QMailFolderId& folderId)
{
    emit d->moveToFolder(action, ids, folderId);
}

/*!
    Asynchronous version of QMailDisconnected::flagMessages()

    Disconnected flags the list of messages identified by \a ids, setting the flags specified by the bit mask \a setMask 
    to on and setting the flags set by the bit mask \a unsetMask to off.
    
    For example this function may be used to mark messages as important.

    The flagging operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().
            
    The request has the identifier \a action.
    
    \sa QMailDisconnected::flagMessages()
*/
void QMailMessageServer::flagMessages(quint64 action, const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask)
{
    emit d->flagMessages(action, ids, setMask, unsetMask);
}

/*!
    Asynchronous version of QMailDisconnected::restoreToPreviousFolder()

    Updates all QMailMessages identified by the key \a key to move the messages back to the
    previous folder they were contained by.
        
    The request has the identifier \a action.
    
    \sa QMailDisconnected::restoreToPreviousFolder(), QMailMessageServer::moveToFolder(), QMailMessageServer::moveToStandardFolder()
*/
void QMailMessageServer::restoreToPreviousFolder(quint64 action, const QMailMessageKey& key)
{
    emit d->restoreToPreviousFolder(action, key);
}

/*!
    Requests that the MessageServer search for messages that meet the criteria encoded
    in \a filter.  If \a bodyText is non-empty, messages containing the specified text 
    in their content will also be matched.  If \a spec is 
    \l{QMailSearchAction::Remote}{Remote} then the MessageServer will extend the search
    to consider messages held at external servers that are not present on the local device.
    If \a sort is not empty, the external service will return matching messages in 
    the ordering indicated by the sort criterion if possible.

    The identifiers of all matching messages are returned via matchingMessageIds() signals.

    The request has the identifier \a action.

    If a remote search is specified then this function requires the device to be online, 
    it may initiate communication with external servers.

    \sa matchingMessageIds(), messagesCount(), remainingMessagesCount()
*/
void QMailMessageServer::searchMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort)
{
    emit d->searchMessages(action, filter, bodyText, spec, sort);
}

/*!
    Requests that the MessageServer search for messages that meet the criteria encoded
    in \a filter.  If \a bodyText is non-empty, messages containing the specified text 
    in their content will also be matched.  If \a spec is 
    \l{QMailSearchAction::Remote}{Remote} then the MessageServer will extend the search
    to consider messages held at external servers that are not present on the local device.

    A maximum of \a limit messages will be retrieved from the remote server.

    If \a sort is not empty, the external service will return matching messages in 
    the ordering indicated by the sort criterion if possible.

    The identifiers of all matching messages are returned via matchingMessageIds() signals.

    The request has the identifier \a action.

    If a remote search is specified then this function requires the device to be online, 
    it may initiate communication with external servers.

    \sa matchingMessageIds(), messagesCount(), remainingMessagesCount()
*/
void QMailMessageServer::searchMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort)
{
    emit d->searchMessages(action, filter, bodyText, spec, limit, sort);
}

/*!
    Requests that the MessageServer counts the number of messages that match the criteria 
    specified by \a filter by on the device and remote servers.  If \a bodyText is non-empty, 
    messages containing the specified text in their content will also be matched.

    The count of all matching messages is returned via a messagesCount() signal.

    The request has the identifier \a action.

    This function requires the device to be online, it may initiate communication 
    with external servers.

    \sa messagesCount()
*/
void QMailMessageServer::countMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText)
{
    emit d->countMessages(action, filter, bodyText);
}

/*!
    Requests that the MessageServer cancel any pending search operations for the request identified by \a action.

    This method is obsolete, use cancel transfer instead.
*/
void QMailMessageServer::cancelSearch(quint64 action)
{
    emit d->cancelTransfer(action);
}

/*!
    Requests that the MessageServer shutdown and terminate
*/
void QMailMessageServer::shutdown()
{
    emit d->shutdown();
}

/*!
    Requests that the MessageServer emits a list of currently executing actions
*/
void QMailMessageServer::listActions()
{
    emit d->listActions();
}

/*!
    Requests that the MessageServer forward the protocol-specific request \a request
    to the QMailMessageSource configured for the account identified by \a accountId.
    The request, identified by \a action, may have associated \a data, in a protocol-specific form.
*/
void QMailMessageServer::protocolRequest(quint64 action, const QMailAccountId &accountId, const QString &request, const QVariant &data)
{
    emit d->protocolRequest(action, accountId, request, data);
}

Q_IMPLEMENT_USER_METATYPE_TYPEDEF(QMailMessageCountMap, QMailMessageCountMap)

/*!
    \fn bool QMailMessageServer::connectionDown()

    Signal that is emitted when the connection to the messageserver has been destroyed.

    \sa reconnectionTimeout()
*/

/*!
    \fn bool QMailMessageServer::reconnectionTimeout()

    Signal that is emitted when the connection to the messageserver has been lost.

    \sa connectionDown()
*/

/*!
    \fn void QMailMessageServer::messagesAdded(quint64 action, const QMailMessageIdList& ids);

    Signal that is emitted when messages have been asynchronously added to the message store.
    
    \a action is the identifier of the request that caused the messages to be added, and \a ids
    is a list of identifiers of messages that have been added.

    \sa QMailStorageAction::addMessages()
*/

/*!
    \fn void QMailMessageServer::messagesUpdated(quint64 action, const QMailMessageIdList& ids);

    Signal that is emitted when messages have been asynchronously updated in the message store.

    \a action is the identifier of the request that caused the messages to be updated, and \a ids
    is a list of identifiers of messages that have been updated.

    \sa QMailStorageAction::updateMessages()
*/

#include "qmailmessageserver.moc"

