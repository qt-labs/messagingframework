/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef SERVICEHANDLER_H
#define SERVICEHANDLER_H

#include <QByteArray>
#include <QLinkedList>
#include <QList>
#include <qmailmessageserver.h>
#include <qmailmessageservice.h>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

class QMailServiceConfiguration;

class ServiceHandler : public QObject
{
    Q_OBJECT

public:
    ServiceHandler(QObject* parent);
    ~ServiceHandler();

public slots:
    void transmitMessages(quint64 action, const QMailAccountId &accountId);
    void retrieveFolderList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);
    void retrieveMessages(quint64, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(quint64, const QMailMessagePart::Location &partLocation);
    void retrieveMessageRange(quint64, const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(quint64, const QMailMessagePart::Location &partLocation, uint minimum);
    void retrieveAll(quint64, const QMailAccountId &accountId);
    void exportUpdates(quint64, const QMailAccountId &accountId);
    void synchronize(quint64, const QMailAccountId &accountId);
    void deleteMessages(quint64 action, const QMailMessageIdList& mailList, QMailStore::MessageRemovalOption);
    void copyMessages(quint64 action, const QMailMessageIdList& mailList, const QMailFolderId &destination);
    void moveMessages(quint64 action, const QMailMessageIdList& mailList, const QMailFolderId &destination);
    void cancelTransfer(quint64 action);
    void searchMessages(quint64 action, const QMailMessageKey &filter, const QString &bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);
    void cancelSearch(quint64);
    void shutdown();
    void protocolRequest(quint64 action, const QMailAccountId &accountId, const QString &request, const QVariant &data);

signals:
    void statusChanged(quint64 action, const QMailServiceAction::Status status);
    void availabilityChanged(quint64 action, bool available);
    void connectivityChanged(quint64 action, QMailServiceAction::Connectivity connectivity);
    void activityChanged(quint64 action, QMailServiceAction::Activity activity);
    void progressChanged(quint64 action, uint progress, uint total);

    void retrievalCompleted(quint64 action);

    void messagesTransmitted(quint64 action, const QMailMessageIdList&);
    void transmissionCompleted(quint64 action);

    void messagesDeleted(quint64 action, const QMailMessageIdList&);
    void messagesCopied(quint64 action, const QMailMessageIdList&);
    void messagesMoved(quint64 action, const QMailMessageIdList&);
    void storageActionCompleted(quint64 action);

    void matchingMessageIds(quint64 action, const QMailMessageIdList&);
    void remoteSearchCompleted(quint64 action);
    void searchCompleted(quint64 action);

    void protocolResponse(quint64 action, const QString &response, const QVariant &data);
    void protocolRequestCompleted(quint64 action);

    void newMessagesAvailable();

private slots:
    void statusChanged(const QMailServiceAction::Status);
    void availabilityChanged(bool);
    void connectivityChanged(QMailServiceAction::Connectivity);
    void activityChanged(QMailServiceAction::Activity);
    void progressChanged(uint, uint );
    void actionCompleted(bool);

    void messagesTransmitted(const QMailMessageIdList&);

    void messagesDeleted(const QMailMessageIdList&);
    void messagesCopied(const QMailMessageIdList&);
    void messagesMoved(const QMailMessageIdList&);
    void messagesPrepared(const QMailMessageIdList&);
    void matchingMessageIds(const QMailMessageIdList&);

    void protocolResponse(const QString &response, const QVariant &data);

    void accountsAdded(const QMailAccountIdList &);
    void accountsUpdated(const QMailAccountIdList &);
    void accountsRemoved(const QMailAccountIdList &);

    void continueSearch();
    void finaliseSearch(quint64 action);

    void dispatchRequest();

    void expireAction();

private:
    QMailAccountId transmissionAccountId(const QMailAccountId &accountId) const;

    void registerAccountServices(const QMailAccountIdList &ids);
    void deregisterAccountServices(const QMailAccountIdList &ids);

    void reregisterAccountServices(const QMailAccountIdList &ids);

    void registerAccountSource(const QMailAccountId &accountId, QMailMessageSource *source, QMailMessageService *service);
    QMailMessageSource *accountSource(const QMailAccountId &accountId) const;

    void registerAccountSink(const QMailAccountId &accountId, QMailMessageSink *sink, QMailMessageService *service);
    QMailMessageSink *accountSink(const QMailAccountId &accountId) const;

    void registerAccountService(const QMailAccountId &accountId, const QMailServiceConfiguration &svcCfg);
    QMailMessageService *createService(const QString &service, const QMailAccountId &accountId);

    bool serviceAvailable(QMailMessageService *service) const;

    QSet<QMailMessageService*> sourceServiceSet(const QMailAccountId &id) const;
    QSet<QMailMessageService*> sourceServiceSet(const QSet<QMailAccountId> &ids) const;

    QSet<QMailMessageService*> sinkServiceSet(const QMailAccountId &id) const;
    QSet<QMailMessageService*> sinkServiceSet(const QSet<QMailAccountId> &ids) const;

    quint64 sourceAction(QMailMessageSource *source) const;
    quint64 sinkAction(QMailMessageSink *sink) const;

    quint64 serviceAction(QMailMessageService *service) const;

    typedef bool (ServiceHandler::*RequestServicer)(quint64, const QByteArray &);
    typedef void (ServiceHandler::*CompletionSignal)(quint64);

    void enqueueRequest(quint64 action, const QByteArray &data, const QSet<QMailMessageService*> &services, RequestServicer servicer, CompletionSignal completion);

    bool dispatchTransmitMessages(quint64 action, const QByteArray& data);
    bool dispatchRetrieveFolderListAccount(quint64, const QByteArray &data);
    bool dispatchRetrieveFolderList(quint64, const QByteArray &data);
    bool dispatchRetrieveMessageList(quint64, const QByteArray &data);
    bool dispatchRetrieveMessages(quint64, const QByteArray &data);
    bool dispatchRetrieveMessagePart(quint64, const QByteArray &data);
    bool dispatchRetrieveMessageRange(quint64, const QByteArray &data);
    bool dispatchRetrieveMessagePartRange(quint64, const QByteArray &data);
    bool dispatchRetrieveAll(quint64, const QByteArray &data);
    bool dispatchExportUpdates(quint64, const QByteArray &data);
    bool dispatchSynchronize(quint64, const QByteArray &data);
    bool dispatchDiscardMessages(quint64 action, const QByteArray &data);
    bool dispatchDeleteMessages(quint64 action, const QByteArray &data);
    bool dispatchCopyMessages(quint64 action, const QByteArray &data);
    bool dispatchCopyToLocal(quint64 action, const QByteArray &data);
    bool dispatchMoveMessages(quint64 action, const QByteArray &data);
    bool dispatchMoveToTrash(quint64 action, const QByteArray &data);
    bool dispatchSearchMessages(quint64 action, const QByteArray &data);
    bool dispatchProtocolRequest(quint64 action, const QByteArray &data);

    void reportFailure(quint64, QMailServiceAction::Status::ErrorCode, const QString& = QString(), const QMailAccountId& = QMailAccountId(), const QMailFolderId& = QMailFolderId(), const QMailMessageId& = QMailMessageId());
    void reportFailure(quint64, const QMailServiceAction::Status);

    void activateAction(quint64, const QSet<QMailMessageService*> &, CompletionSignal);
    void updateAction(quint64);

    QMap<QPair<QMailAccountId, QString>, QMailMessageService*> serviceMap;
    QMap<QMailAccountId, QMailMessageSource*> sourceMap;
    QMap<QMailAccountId, QMailMessageSink*> sinkMap;
    QMap<QMailMessageSource*, QMailMessageService*> sourceService;
    QMap<QMailMessageSink*, QMailMessageService*> sinkService;
    QMap<QMailAccountId, QMailAccountId> masterAccount;

    QSet<QMailMessageService*> mUnavailableServices;

    class ActionData
    {
    public:
        QSet<QMailMessageService*> services;
        CompletionSignal completion;
        QTime expiry;
    };
    
    QMap<quint64, ActionData> mActiveActions;
    QLinkedList<quint64> mActionExpiry;

    QMap<QMailMessageService*, quint64> mServiceAction;

    enum { ExpiryPeriod = 30 * 1000 };

    struct Request 
    {
        quint64 action;
        QByteArray data;
        QSet<QMailMessageService*> services;
        RequestServicer servicer;
        CompletionSignal completion;
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
};

#endif
