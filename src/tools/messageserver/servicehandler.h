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

#ifndef SERVICEHANDLER_H
#define SERVICEHANDLER_H

#include <QByteArray>
#include <QFile>
#include <QLinkedList>
#include <QList>
#include <qmailmessageserver.h>
#include <qmailmessageservice.h>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QPointer>

class QMailServiceConfiguration;

class ServiceHandler : public QObject
{
    Q_OBJECT

public:
    ServiceHandler(QObject* parent);
    ~ServiceHandler();

public slots:
    void transmitMessages(quint64 action, const QMailAccountId &accountId);
    void transmitMessage(quint64, const QMailMessageId &messageId);
    void retrieveFolderList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);
    void retrieveMessageLists(quint64, const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort);
    void retrieveNewMessages(quint64, const QMailAccountId &accountId, const QMailFolderIdList &folderIds);
    void createStandardFolders(quint64, const QMailAccountId &accountId);
    void retrieveMessages(quint64, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(quint64, const QMailMessagePart::Location &partLocation);
    void retrieveMessageRange(quint64, const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(quint64, const QMailMessagePart::Location &partLocation, uint minimum);
    void retrieveAll(quint64, const QMailAccountId &accountId);
    void exportUpdates(quint64, const QMailAccountId &accountId);
    void synchronize(quint64, const QMailAccountId &accountId);
    void onlineDeleteMessages(quint64 action, const QMailMessageIdList& mailList, QMailStore::MessageRemovalOption);
    void discardMessages(quint64 action, QMailMessageIdList messageIds);
    void onlineCopyMessages(quint64 action, const QMailMessageIdList& mailList, const QMailFolderId &destination);
    void onlineMoveMessages(quint64 action, const QMailMessageIdList& mailList, const QMailFolderId &destination);
    void onlineFlagMessagesAndMoveToStandardFolder(quint64 action, const QMailMessageIdList& mailList, quint64 setMask, quint64 unsetMask);
    void addMessages(quint64 action, const QString &filename);
    void addMessages(quint64 action, const QMailMessageMetaDataList &messages);
    void updateMessages(quint64 action, const QString &filename);
    void updateMessages(quint64 action, const QMailMessageMetaDataList &messages);
    void deleteMessages(quint64 action, const QMailMessageIdList &ids);
    void rollBackUpdates(quint64 action, const QMailAccountId &mailAccountId);
    void moveToStandardFolder(quint64 action, const QMailMessageIdList& ids, quint64 standardFolder);
    void moveToFolder(quint64 action, const QMailMessageIdList& ids, const QMailFolderId& folderId);
    void flagMessages(quint64 action, const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask);
    void restoreToPreviousFolder(quint64, const QMailMessageKey& key);

    void onlineCreateFolder(quint64 action, const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    void onlineRenameFolder(quint64 action, const QMailFolderId &folderId, const QString &name);
    void onlineDeleteFolder(quint64 action, const QMailFolderId &folderId);
    void cancelTransfer(quint64 action);
    void searchMessages(quint64 action, const QMailMessageKey &filter, const QString &bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);
    void searchMessages(quint64 action, const QMailMessageKey &filter, const QString &bodyText, QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort);
    void countMessages(quint64 action, const QMailMessageKey &filter, const QString &bodyText);
    void cancelLocalSearch(quint64 action);
    void shutdown();
    void listActions();
    void protocolRequest(quint64 action, const QMailAccountId &accountId, const QString &request, const QVariant &data);

signals:
    void actionStarted(QMailActionData action);

    void statusChanged(quint64 action, const QMailServiceAction::Status status);
    void availabilityChanged(quint64 action, bool available);
    void connectivityChanged(quint64 action, QMailServiceAction::Connectivity connectivity);
    void activityChanged(quint64 action, QMailServiceAction::Activity activity);
    void progressChanged(quint64 action, uint progress, uint total);

    void retrievalCompleted(quint64 action);

    void messagesTransmitted(quint64 action, const QMailMessageIdList&);
    void messagesFailedTransmission(quint64 action, const QMailMessageIdList&, QMailServiceAction::Status::ErrorCode);
    void transmissionCompleted(quint64 action);

    void messagesDeleted(quint64 action, const QMailMessageIdList&);
    void messagesCopied(quint64 action, const QMailMessageIdList&);
    void messagesMoved(quint64 action, const QMailMessageIdList&);
    void messagesFlagged(quint64 action, const QMailMessageIdList&);

    void messagesAdded(quint64 action, const QMailMessageIdList&);
    void messagesUpdated(quint64 action, const QMailMessageIdList&);

    void folderCreated(quint64 action, const QMailFolderId&);
    void folderRenamed(quint64 action, const QMailFolderId&);
    void folderDeleted(quint64 action, const QMailFolderId&); 

    void storageActionCompleted(quint64 action);

    void matchingMessageIds(quint64 action, const QMailMessageIdList&);
    void remainingMessagesCount(quint64 action, uint);
    void messagesCount(quint64 action, uint);
    void remoteSearchCompleted(quint64 action);
    void searchCompleted(quint64 action);

    void actionsListed(const QMailActionDataList &actions);

    void protocolResponse(quint64 action, const QString &response, const QVariant &data);
    void protocolRequestCompleted(quint64 action);

    void newMessagesAvailable();

private slots:
    void statusChanged(const QMailServiceAction::Status);
    void statusChanged(const QMailServiceAction::Status, quint64);
    void availabilityChanged(bool);
    void availabilityChanged(bool, quint64);
    void connectivityChanged(QMailServiceAction::Connectivity);
    void connectivityChanged(QMailServiceAction::Connectivity, quint64);
    void activityChanged(QMailServiceAction::Activity);
    void activityChanged(QMailServiceAction::Activity, quint64);
    void progressChanged(uint, uint );
    void progressChanged(uint, uint, quint64);
    void actionCompleted(bool);
    void actionCompleted(bool, quint64);
    void actionCompleted(bool, QMailMessageService *, quint64);

    void messagesTransmitted(const QMailMessageIdList&);
    void messagesTransmitted(const QMailMessageIdList&, quint64);
    void messagesFailedTransmission(const QMailMessageIdList&, QMailServiceAction::Status::ErrorCode);
    void messagesFailedTransmission(const QMailMessageIdList&, QMailServiceAction::Status::ErrorCode, quint64);

    void messagesDeleted(const QMailMessageIdList&);
    void messagesDeleted(const QMailMessageIdList& , quint64);
    void messagesCopied(const QMailMessageIdList&);
    void messagesCopied(const QMailMessageIdList&, quint64);
    void messagesMoved(const QMailMessageIdList&);
    void messagesMoved(const QMailMessageIdList&, quint64);
    void messagesFlagged(const QMailMessageIdList&);
    void messagesFlagged(const QMailMessageIdList&, quint64);
    void messagesPrepared(const QMailMessageIdList&);
    void messagesPrepared(const QMailMessageIdList&, quint64);
    void matchingMessageIds(const QMailMessageIdList&);
    void matchingMessageIds(const QMailMessageIdList&, quint64);
    void remainingMessagesCount(uint count);
    void remainingMessagesCount(uint count, quint64);
    void messagesCount(uint count);
    void messagesCount(uint count, quint64);

    void protocolResponse(const QString &response, const QVariant &data);
    void protocolResponse(const QString &response, const QVariant &data, quint64);

    void accountsAdded(const QMailAccountIdList &);
    void accountsUpdated(const QMailAccountIdList &);
    void accountsRemoved(const QMailAccountIdList &);

    void continueSearch();

    void dispatchRequest();

    void expireAction();

    void reportFailures();

private:
    enum SearchType {
        NoLimit = 0,
        Limit = 1,
        Count = 2
    };

    void searchMessages(quint64 action, const QMailMessageKey &filter, const QString &bodyText, QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort, ServiceHandler::SearchType searchType);

    QMailAccountId transmissionAccountId(const QMailAccountId &accountId) const;

    void registerAccountServices(const QMailAccountIdList &ids);
    void deregisterAccountServices(const QMailAccountIdList &ids, QMailServiceAction::Status::ErrorCode code, const QString &text);
    void deregisterAccountService(const QMailAccountId &id, const QString &serviceName, QMailServiceAction::Status::ErrorCode code, const QString &text);
    void removeServiceFromActions(QMailMessageService *removeService);

    void reregisterAccountServices(QMailAccountIdList ids, QMailServiceAction::Status::ErrorCode code, const QString &text);

    void registerAccountSource(const QMailAccountId &accountId, QMailMessageSource *source, QMailMessageService *service);
    QMailMessageSource *accountSource(const QMailAccountId &accountId) const;

    void registerAccountSink(const QMailAccountId &accountId, QMailMessageSink *sink, QMailMessageService *service);
    QMailMessageSink *accountSink(const QMailAccountId &accountId) const;

    void registerAccountService(const QMailAccountId &accountId, const QMailServiceConfiguration &svcCfg);
    QMailMessageService *createService(const QString &service, const QMailAccountId &accountId);

    struct Request;
    bool servicesAvailable(const Request &services) const;
    bool serviceAvailable(QPointer<QMailMessageService> service) const;

    QSet<QMailMessageService*> sourceServiceSet(const QMailAccountId &id) const;
    QSet<QMailMessageService*> sourceServiceSet(const QSet<QMailAccountId> &ids) const;

    QSet<QMailMessageService*> sinkServiceSet(const QMailAccountId &id) const;
    QSet<QMailMessageService*> sinkServiceSet(const QSet<QMailAccountId> &ids) const;

    quint64 sourceAction(QMailMessageSource *source) const;
    quint64 sinkAction(QMailMessageSink *sink) const;

    quint64 serviceAction(QMailMessageService *service) const;

    typedef bool (ServiceHandler::*RequestServicer)(quint64, const QByteArray &);
    typedef void (ServiceHandler::*CompletionSignal)(quint64);

    void enqueueRequest(quint64 action, const QByteArray &data, const QSet<QMailMessageService*> &services, RequestServicer servicer, CompletionSignal completion, QMailServerRequestType description, const QSet<QMailMessageService*> &preconditions = QSet<QMailMessageService*>());

    bool dispatchPrepareMessages(quint64 action, const QByteArray& data);
    bool dispatchTransmitMessages(quint64 action, const QByteArray& data);
    bool dispatchTransmitMessage(quint64 action, const QByteArray& data);
    bool dispatchRetrieveFolderListAccount(quint64, const QByteArray &data);
    bool dispatchRetrieveFolderList(quint64, const QByteArray &data);
    bool dispatchRetrieveMessageList(quint64, const QByteArray &data);
    bool dispatchRetrieveMessageLists(quint64, const QByteArray &data);
    bool dispatchRetrieveNewMessages(quint64, const QByteArray &data);
    bool dispatchCreateStandardFolders(quint64, const QByteArray &data);
    bool dispatchRetrieveMessages(quint64, const QByteArray &data);
    bool dispatchRetrieveMessagePart(quint64, const QByteArray &data);
    bool dispatchRetrieveMessageRange(quint64, const QByteArray &data);
    bool dispatchRetrieveMessagePartRange(quint64, const QByteArray &data);
    bool dispatchRetrieveAll(quint64, const QByteArray &data);
    bool dispatchExportUpdates(quint64, const QByteArray &data);
    bool dispatchSynchronize(quint64, const QByteArray &data);
    bool dispatchOnlineDeleteMessages(quint64 action, const QByteArray &data);
    bool dispatchOnlineCopyMessages(quint64 action, const QByteArray &data);
    bool dispatchCopyToLocal(quint64 action, const QByteArray &data);
    bool dispatchOnlineMoveMessages(quint64 action, const QByteArray &data);
    bool dispatchOnlineFlagMessagesAndMoveToStandardFolder(quint64 action, const QByteArray &data);
    bool dispatchOnlineCreateFolder(quint64 action, const QByteArray &data);
    bool dispatchOnlineDeleteFolder(quint64 action, const QByteArray &data);
    bool dispatchOnlineRenameFolder(quint64 action, const QByteArray &data);
    bool dispatchSearchMessages(quint64 action, const QByteArray &data);
    bool dispatchProtocolRequest(quint64 action, const QByteArray &data);

    void reportFailure(quint64, QMailServiceAction::Status::ErrorCode, const QString& = QString(), const QMailAccountId& = QMailAccountId(), const QMailFolderId& = QMailFolderId(), const QMailMessageId& = QMailMessageId());
    void reportFailure(quint64, const QMailServiceAction::Status);

    void updateAction(quint64);

    void setRetrievalInProgress(const QMailAccountId &id, bool inProgress);
    void setTransmissionInProgress(const QMailAccountId &id, bool inProgress);
    void addOrUpdateMessages(quint64 action, const QString &filename, bool add);

    QMap<QPair<QMailAccountId, QString>, QPointer<QMailMessageService> > serviceMap;
    QMap<QMailAccountId, QMailMessageSource*> sourceMap;
    QMap<QMailAccountId, QMailMessageSink*> sinkMap;
    QMap<QMailMessageSource*, QPointer<QMailMessageService> > sourceService;
    QMap<QMailMessageSink*, QPointer<QMailMessageService> > sinkService;
    QMap<QMailAccountId, QMailAccountId> masterAccount;

    QSet<QMailMessageService*> mUnavailableServices;

    class ActionData
    {
    public:
        QMailServerRequestType description;
        QSet<QPointer<QMailMessageService> > services;
        CompletionSignal completion;
        uint unixTimeExpiry;
        bool reported;
        // Fields for QMailActionInfo
        uint progressTotal;
        uint progressCurrent;
        QMailServiceAction::Status status;
    };
    
    QMap<quint64, ActionData> mActiveActions;
    QLinkedList<quint64> mActionExpiry;

    QMap<QPointer<QMailMessageService>, quint64> mServiceAction;

    static const int ExpirySeconds = 120;

    struct Request 
    {
        quint64 action;
        QByteArray data;
        QSet<QPointer<QMailMessageService> > services;
        QSet<QPointer<QMailMessageService> > preconditions;
        RequestServicer servicer;
        CompletionSignal completion;
        QMailServerRequestType description;
    };

    QList<Request> mRequests;

    class MessageSearch
    {
    public:
        MessageSearch(quint64 action, const QMailMessageIdList &ids, const QString &text);

        quint64 action() const;

        const QString &bodyText() const;

        bool pending() const;
        void inProgress();

        bool isEmpty() const;

        uint total() const;
        uint progress() const;

        QMailMessageIdList takeBatch();

    private:
        quint64 _action;
        QMailMessageIdList _ids;
        QString _text;
        bool _active;
        uint _total;
        uint _progress;
    };

    QList<MessageSearch> mSearches;
    QMailMessageIdList mMatchingIds;
    QMailMessageIdList mSentIds;

    QFile _requestsFile;
    QSet<quint64> _outstandingRequests;
    QList<quint64> _failedRequests;

    QSet<QMailAccountId> _retrievalAccountIds;
    QSet<QMailAccountId> _transmissionAccountIds;
};

#endif
