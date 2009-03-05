/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef MAILMESSAGECLIENT_H
#define MAILMESSAGECLIENT_H

#include "qmailmessageserver.h"
#include <QObject>
#include <qmailipc.h>

// The back-end corresponding to the front-end in QMailMessageServer
class MailMessageClient : public QObject
{
    Q_OBJECT

    friend class MessageServer;

public:
    MailMessageClient(QObject* parent);
    ~MailMessageClient();

private:
    // Disallow copying
    MailMessageClient(const MailMessageClient&);
    void operator=(const MailMessageClient&);

signals:
    void newCountChanged(const QMailMessageCountMap&);
    void acknowledgeNewMessages(const QMailMessageTypeList&);

    void transmitMessages(quint64, const QMailAccountId &accountId);

    void retrieveFolderList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);

    void retrieveMessages(quint64, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(quint64, const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(quint64, const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(quint64, const QMailMessagePart::Location &partLocation, uint minimum);

    void retrieveAll(quint64, const QMailAccountId &accountId);
    void exportUpdates(quint64, const QMailAccountId &accountId);

    void synchronize(quint64, const QMailAccountId &accountId);

    void deleteMessages(quint64, const QMailMessageIdList&, QMailStore::MessageRemovalOption);

    void copyMessages(quint64, const QMailMessageIdList&, const QMailFolderId&);
    void moveMessages(quint64, const QMailMessageIdList&, const QMailFolderId&);

    void cancelTransfer(quint64);

    void searchMessages(quint64, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);

    void cancelSearch(quint64);

    void shutdown();

    void protocolRequest(quint64, const QMailAccountId &accountId, const QString &request, const QVariant &data);

    void activityChanged(quint64, QMailServiceAction::Activity);
    void connectivityChanged(quint64, QMailServiceAction::Connectivity);
    void statusChanged(quint64, const QMailServiceAction::Status);
    void progressChanged(quint64, uint, uint);

    void retrievalCompleted(quint64);

    void messagesTransmitted(quint64, const QMailMessageIdList&);
    void transmissionCompleted(quint64);

    void messagesDeleted(quint64, const QMailMessageIdList&);
    void messagesCopied(quint64, const QMailMessageIdList&);
    void messagesMoved(quint64, const QMailMessageIdList&);
    void storageActionCompleted(quint64);

    void matchingMessageIds(quint64, const QMailMessageIdList&);
    void searchCompleted(quint64);

    void protocolResponse(quint64, const QString &response, const QVariant &data);
    void protocolRequestCompleted(quint64);

private:
    QCopAdaptor* adaptor;
};

#endif
