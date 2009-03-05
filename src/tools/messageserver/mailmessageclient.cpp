/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "mailmessageclient.h"

static bool connectIpc( QObject *sender, const QByteArray& signal,
        QObject *receiver, const QByteArray& member)
{
    return QCopAdaptor::connect(sender,signal,receiver,member);
}

MailMessageClient::MailMessageClient(QObject* parent)
    : QObject(parent),
      adaptor(new QCopAdaptor("QPE/QMailMessageServer",this))
{
    connectIpc(this, SIGNAL(newCountChanged(QMailMessageCountMap)),
                              adaptor, MESSAGE(newCountChanged(QMailMessageCountMap)));
    connectIpc(adaptor, MESSAGE(acknowledgeNewMessages(QMailMessageTypeList)),
                              this, SIGNAL(acknowledgeNewMessages(QMailMessageTypeList)));

    connectIpc(this, SIGNAL(activityChanged(quint64, QMailServiceAction::Activity)),
               adaptor, MESSAGE(activityChanged(quint64, QMailServiceAction::Activity)));
    connectIpc(this, SIGNAL(connectivityChanged(quint64, QMailServiceAction::Connectivity)),
               adaptor, MESSAGE(connectivityChanged(quint64, QMailServiceAction::Connectivity)));
    connectIpc(this, SIGNAL(statusChanged(quint64, const QMailServiceAction::Status)),
               adaptor, MESSAGE(statusChanged(quint64, const QMailServiceAction::Status)));
    connectIpc(this, SIGNAL(progressChanged(quint64, uint, uint)),
               adaptor, MESSAGE(progressChanged(quint64, uint, uint)));
    connectIpc(this, SIGNAL(transmissionCompleted(quint64)), 
               adaptor, MESSAGE(transmissionCompleted(quint64)));
    connectIpc(this, SIGNAL(messagesTransmitted(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(messagesTransmitted(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(retrievalCompleted(quint64)),
               adaptor, MESSAGE(retrievalCompleted(quint64)));
    connectIpc(this, SIGNAL(messagesDeleted(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(messagesDeleted(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(messagesCopied(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(messagesCopied(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(messagesMoved(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(messagesMoved(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(storageActionCompleted(quint64)), 
               adaptor, MESSAGE(storageActionCompleted(quint64)));
    connectIpc(this, SIGNAL(matchingMessageIds(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(matchingMessageIds(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(searchCompleted(quint64)), 
               adaptor, MESSAGE(searchCompleted(quint64)));
    connectIpc(this, SIGNAL(protocolResponse(quint64, QString, QVariant)),
               adaptor, MESSAGE(protocolResponse(quint64, QString, QVariant)));
    connectIpc(this, SIGNAL(protocolRequestCompleted(quint64)),
               adaptor, MESSAGE(protocolRequestCompleted(quint64)));

    connectIpc(adaptor, MESSAGE(transmitMessages(quint64, QMailAccountId)),
               this, SIGNAL(transmitMessages(quint64, QMailAccountId)));
    connectIpc(adaptor, MESSAGE(retrieveFolderList(quint64, QMailAccountId, QMailFolderId, bool)),
               this, SIGNAL(retrieveFolderList(quint64, QMailAccountId, QMailFolderId, bool)));
    connectIpc(adaptor, MESSAGE(retrieveMessageList(quint64, QMailAccountId, QMailFolderId, uint, QMailMessageSortKey)),
               this, SIGNAL(retrieveMessageList(quint64, QMailAccountId, QMailFolderId, uint, QMailMessageSortKey)));
    connectIpc(adaptor, MESSAGE(retrieveMessages(quint64, QMailMessageIdList, QMailRetrievalAction::RetrievalSpecification)),
               this, SIGNAL(retrieveMessages(quint64, QMailMessageIdList, QMailRetrievalAction::RetrievalSpecification)));
    connectIpc(adaptor, MESSAGE(retrieveMessagePart(quint64, QMailMessagePart::Location)),
               this, SIGNAL(retrieveMessagePart(quint64, QMailMessagePart::Location)));
    connectIpc(adaptor, MESSAGE(retrieveMessageRange(quint64, QMailMessageId, uint)),
               this, SIGNAL(retrieveMessageRange(quint64, QMailMessageId, uint)));
    connectIpc(adaptor, MESSAGE(retrieveMessagePartRange(quint64, QMailMessagePart::Location, uint)),
               this, SIGNAL(retrieveMessagePartRange(quint64, QMailMessagePart::Location, uint)));
    connectIpc(adaptor, MESSAGE(retrieveAll(quint64, QMailAccountId)),
               this, SIGNAL(retrieveAll(quint64, QMailAccountId)));
    connectIpc(adaptor, MESSAGE(exportUpdates(quint64, QMailAccountId)),
               this, SIGNAL(exportUpdates(quint64, QMailAccountId)));
    connectIpc(adaptor, MESSAGE(synchronize(quint64, QMailAccountId)),
               this, SIGNAL(synchronize(quint64, QMailAccountId)));
    connectIpc(adaptor, MESSAGE(deleteMessages(quint64, QMailMessageIdList, QMailStore::MessageRemovalOption)),
               this, SIGNAL(deleteMessages(quint64, QMailMessageIdList, QMailStore::MessageRemovalOption)));
    connectIpc(adaptor, MESSAGE(copyMessages(quint64, QMailMessageIdList, QMailFolderId)),
               this, SIGNAL(copyMessages(quint64, QMailMessageIdList, QMailFolderId)));
    connectIpc(adaptor, MESSAGE(moveMessages(quint64, QMailMessageIdList, QMailFolderId)),
               this, SIGNAL(moveMessages(quint64, QMailMessageIdList, QMailFolderId)));
    connectIpc(adaptor, MESSAGE(cancelTransfer(quint64)),
               this, SIGNAL(cancelTransfer(quint64)));
    connectIpc(adaptor, MESSAGE(cancelSearch(quint64)),
               this, SIGNAL(cancelSearch(quint64)));
    connectIpc(adaptor, MESSAGE(shutdown()),
               this, SIGNAL(shutdown()));
    connectIpc(adaptor, MESSAGE(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, QMailMessageSortKey)),
               this, SIGNAL(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, QMailMessageSortKey)));
    connectIpc(adaptor, MESSAGE(protocolRequest(quint64, QMailAccountId, QString, QVariant)),
               this, SIGNAL(protocolRequest(quint64, QMailAccountId, QString, QVariant)));
}

MailMessageClient::~MailMessageClient()
{
}

