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

#ifndef SERVICEHANDLER_H
#define SERVICEHANDLER_H

#include <QFile>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QPointer>

#include <qmailmessageserver.h>
#include <qmailmessageservice.h>

class QMailServiceConfiguration;
class Request;

enum SearchType {
    NoLimit = 0,
    Limit = 1,
    Count = 2
};

class ServiceHandler : public QObject
{
    Q_OBJECT

public:
    typedef bool (ServiceHandler::*RequestServicer)(Request*);
    typedef void (ServiceHandler::*CompletionSignal)(quint64);

    ServiceHandler(QObject* parent);
    ~ServiceHandler();

public slots:
    void transmitMessages(quint64 action, const QMailAccountId &accountId);
    void transmitMessage(quint64, const QMailMessageId &messageId);
    void retrieveFolderList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId,
                             uint minimum, const QMailMessageSortKey &sort);
    void retrieveMessageLists(quint64, const QMailAccountId &accountId, const QMailFolderIdList &folderIds,
                              uint minimum, const QMailMessageSortKey &sort);
    void retrieveNewMessages(quint64, const QMailAccountId &accountId, const QMailFolderIdList &folderIds);
    void createStandardFolders(quint64, const QMailAccountId &accountId);
    void retrieveMessages(quint64, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(quint64, const QMailMessagePart::Location &partLocation);
    void retrieveMessageRange(quint64, const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(quint64, const QMailMessagePart::Location &partLocation, uint minimum);
    void exportUpdates(quint64, const QMailAccountId &accountId);
    void synchronize(quint64, const QMailAccountId &accountId);
    void onlineDeleteMessages(quint64 action, const QMailMessageIdList& mailList, QMailStore::MessageRemovalOption);
    void discardMessages(quint64 action, QMailMessageIdList messageIds);
    void onlineCopyMessages(quint64 action, const QMailMessageIdList& mailList, const QMailFolderId &destination);
    void onlineMoveMessages(quint64 action, const QMailMessageIdList& mailList, const QMailFolderId &destination);
    void onlineFlagMessagesAndMoveToStandardFolder(quint64 action, const QMailMessageIdList& mailList,
                                                   quint64 setMask, quint64 unsetMask);
    void addMessages(quint64 action, const QMailMessageMetaDataList &messages);
    void updateMessages(quint64 action, const QMailMessageMetaDataList &messages);
    void deleteMessages(quint64 action, const QMailMessageIdList &ids);
    void rollBackUpdates(quint64 action, const QMailAccountId &mailAccountId);
    void moveToStandardFolder(quint64 action, const QMailMessageIdList& ids, quint64 standardFolder);
    void moveToFolder(quint64 action, const QMailMessageIdList& ids, const QMailFolderId& folderId);
    void flagMessages(quint64 action, const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask);
    void restoreToPreviousFolder(quint64, const QMailMessageKey& key);

    void onlineCreateFolder(quint64 action, const QString &name, const QMailAccountId &accountId,
                            const QMailFolderId &parentId);
    void onlineRenameFolder(quint64 action, const QMailFolderId &folderId, const QString &name);
    void onlineDeleteFolder(quint64 action, const QMailFolderId &folderId);
    void onlineMoveFolder(quint64 action, const QMailFolderId &folderId, const QMailFolderId &newParentId);
    void cancelTransfer(quint64 action);
    void searchMessages(quint64 action, const QMailMessageKey &filter, const QString &bodyText,
                        QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);
    void searchMessages(quint64 action, const QMailMessageKey &filter, const QString &bodyText,
                        QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort);
    void countMessages(quint64 action, const QMailMessageKey &filter, const QString &bodyText);
    void cancelLocalSearch(quint64 action);
    void shutdown();
    void listActions();
    void protocolRequest(quint64 action, const QMailAccountId &accountId, const QString &request,
                         const QVariantMap &data);

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
    void folderMoved(quint64 action, const QMailFolderId&);

    void storageActionCompleted(quint64 action);

    void matchingMessageIds(quint64 action, const QMailMessageIdList&);
    void remainingMessagesCount(quint64 action, uint);
    void messagesCount(quint64 action, uint);
    void remoteSearchCompleted(quint64 action);
    void searchCompleted(quint64 action);

    void actionsListed(const QMailActionDataList &actions);

    void protocolResponse(quint64 action, const QString &response, const QVariantMap &data);
    void protocolRequestCompleted(quint64 action);

    void newMessagesAvailable();
    void messageCountUpdated();

    void transmissionReady(quint64 action);
    void retrievalReady(quint64 action);

private slots:
    void onStatusChanged(const QMailServiceAction::Status);
    void onStatusChanged(const QMailServiceAction::Status, quint64);
    void onAvailabilityChanged(bool);
    void emitAvailabilityChanged(bool, quint64);
    void onConnectivityChanged(QMailServiceAction::Connectivity);
    void emitConnectivityChanged(QMailServiceAction::Connectivity, quint64);
    void onActivityChanged(QMailServiceAction::Activity);
    void emitActivityChanged(QMailServiceAction::Activity, quint64);
    void onProgressChanged(uint, uint );
    void onProgressChanged(uint, uint, quint64);
    void onActionCompleted(bool);
    void onActionCompleted(bool, quint64);

    void onMessagesTransmitted(const QMailMessageIdList&);
    void emitMessagesTransmitted(const QMailMessageIdList&, quint64);
    void onMessagesFailedTransmission(const QMailMessageIdList&, QMailServiceAction::Status::ErrorCode);
    void emitMessagesFailedTransmission(const QMailMessageIdList&, QMailServiceAction::Status::ErrorCode, quint64);

    void onMessagesDeleted(const QMailMessageIdList&);
    void emitMessagesDeleted(const QMailMessageIdList& , quint64);
    void onMessagesCopied(const QMailMessageIdList&);
    void emitMessagesCopied(const QMailMessageIdList&, quint64);
    void onMessagesMoved(const QMailMessageIdList&);
    void emitMessagesMoved(const QMailMessageIdList&, quint64);
    void onMessagesFlagged(const QMailMessageIdList&);
    void emitMessagesFlagged(const QMailMessageIdList&, quint64);
    void onMessagesPrepared(const QMailMessageIdList&);
    void emitMessagesPrepared(const QMailMessageIdList&, quint64);
    void onMatchingMessageIds(const QMailMessageIdList&);
    void emitMatchingMessageIds(const QMailMessageIdList&, quint64);
    void onRemainingMessagesCount(uint count);
    void emitRemainingMessagesCount(uint count, quint64);
    void onMessagesCount(uint count);
    void emitMessagesCount(uint count, quint64);

    void onProtocolResponse(const QString &response, const QVariantMap &data);
    void emitProtocolResponse(const QString &response, const QVariantMap &data, quint64);

    void onAccountsAdded(const QMailAccountIdList &);
    void onAccountsUpdated(const QMailAccountIdList &);
    void onAccountsRemoved(const QMailAccountIdList &);

    void continueSearch();

    void dispatchRequest();

    void expireAction();

    void reportPastFailures();

private:
    void handleActionCompleted(bool, QMailMessageService *, quint64);

    void searchMessages(quint64 action, const QMailMessageKey &filter, const QString &bodyText,
                        QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort,
                        SearchType searchType);

    QMailAccountId transmissionAccountId(const QMailAccountId &accountId) const;

    void registerAccountServices(const QMailAccountIdList &ids);
    void deregisterAccountServices(const QMailAccountIdList &ids, QMailServiceAction::Status::ErrorCode code,
                                   const QString &text);
    void deregisterAccountService(const QMailAccountId &id, const QString &serviceName,
                                  QMailServiceAction::Status::ErrorCode code, const QString &text);
    void removeServiceFromActions(QMailMessageService *removeService);

    void reregisterAccountServices(QMailAccountIdList ids, QMailServiceAction::Status::ErrorCode code, const QString &text);

    void registerAccountSource(const QMailAccountId &accountId, QMailMessageSource *source, QMailMessageService *service);
    QMailMessageSource *accountSource(const QMailAccountId &accountId) const;

    void registerAccountSink(const QMailAccountId &accountId, QMailMessageSink *sink, QMailMessageService *service);
    QMailMessageSink *accountSink(const QMailAccountId &accountId) const;

    void registerAccountService(const QMailAccountId &accountId, const QMailServiceConfiguration &svcCfg);
    QMailMessageService *createService(const QString &service, const QMailAccountId &accountId);

    bool servicesAvailable(const Request &services) const;
    bool serviceAvailable(QPointer<QMailMessageService> service) const;

    QSet<QMailMessageService*> sourceServiceSet(const QMailAccountId &id) const;
    QSet<QMailMessageService*> sourceServiceSet(const QSet<QMailAccountId> &ids) const;
    QSet<QMailMessageService*> sourceServiceSet(const QMailAccountIdList &ids) const;

    QSet<QMailMessageService*> sinkServiceSet(const QMailAccountId &id) const;
    QSet<QMailMessageService*> sinkServiceSet(const QSet<QMailAccountId> &ids) const;

    quint64 sourceAction(QMailMessageSource *source) const;
    quint64 sinkAction(QMailMessageSink *sink) const;

    quint64 serviceAction(QMailMessageService *service) const;

    void enqueueRequest(Request *req, quint64 action, const QSet<QMailMessageService*> &services,
                        RequestServicer servicer, CompletionSignal completion,
                        const QSet<QMailMessageService*> &preconditions = QSet<QMailMessageService*>());

    bool dispatchPrepareMessages(Request *req);
    bool dispatchTransmitAccountMessages(Request *req);
    bool dispatchTransmitMessage(Request *req);
    bool dispatchRetrieveFolderListAccount(Request *req);
    bool dispatchRetrieveFolderList(Request *req);
    bool dispatchRetrieveMessageList(Request *req);
    bool dispatchRetrieveMessageLists(Request *req);
    bool dispatchRetrieveNewMessages(Request *req);
    bool dispatchCreateStandardFolders(Request *req);
    bool dispatchRetrieveMessages(Request *req);
    bool dispatchRetrieveMessagePart(Request *req);
    bool dispatchRetrieveMessageRange(Request *req);
    bool dispatchRetrieveMessagePartRange(Request *req);
    bool dispatchExportUpdates(Request *req);
    bool dispatchSynchronize(Request *req);
    bool dispatchOnlineDeleteMessages(Request *req);
    bool dispatchOnlineCopyMessages(Request *req);
    bool dispatchCopyToLocal(Request *req);
    bool dispatchOnlineMoveMessages(Request *req);
    bool dispatchOnlineFlagMessagesAndMoveToStandardFolder(Request *req);
    bool dispatchOnlineCreateFolder(Request *req);
    bool dispatchOnlineDeleteFolder(Request *req);
    bool dispatchOnlineRenameFolder(Request *req);
    bool dispatchOnlineMoveFolder(Request *req);
    bool dispatchSearchMessages(Request *req);
    bool dispatchProtocolRequest(Request *req);

    void reportFailure(quint64, QMailServiceAction::Status::ErrorCode, const QString& = QString(),
                       const QMailAccountId& = QMailAccountId(), const QMailFolderId& = QMailFolderId(),
                       const QMailMessageId& = QMailMessageId());
    void reportFailure(quint64, const QMailServiceAction::Status);

    void updateAction(quint64);

    void setRetrievalInProgress(const QMailAccountId &id, bool inProgress);
    void setTransmissionInProgress(const QMailAccountId &id, bool inProgress);

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
        QMailServerRequestType requestType;
        QSet<QPointer<QMailMessageService> > services;
        CompletionSignal completion;
        quint64 unixTimeExpiry;
        bool reported;
        // Fields for QMailActionInfo
        uint progressTotal;
        uint progressCurrent;
        QMailServiceAction::Status status;
    };

    QMap<quint64, ActionData> mActiveActions;
    QList<quint64> mActionExpiry;

    QMap<QPointer<QMailMessageService>, quint64> mServiceAction;

    QList<QSharedPointer<Request>> mRequests;

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

class Request
{
public:
    Request(QMailServerRequestType requestType) : requestType(requestType) {}
    virtual ~Request() {}

    quint64 action;
    QSet<QPointer<QMailMessageService> > services;
    QSet<QPointer<QMailMessageService> > preconditions;
    ServiceHandler::RequestServicer servicer;
    ServiceHandler::CompletionSignal completion;
    QMailServerRequestType requestType;
};

#endif
