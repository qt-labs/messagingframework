/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGESERVER_H
#define QMAILMESSAGESERVER_H

#include "qmailglobal.h"
#include <QList>
#include "qmailmessage.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"
#include "qmailserviceaction.h"
#include "qmailstore.h"
#include <QMap>
#include <QSharedDataPointer>
#include <QString>
#include <QStringList>
#include "qmailipc.h"

class QMailMessageServerPrivate;

typedef QMap<QMailMessage::MessageType, int> QMailMessageCountMap;

class QTOPIAMAIL_EXPORT QMailMessageServer : public QObject
{
    Q_OBJECT

public:
    QMailMessageServer(QObject* parent = 0);
    ~QMailMessageServer();

private:
    // Disallow copying
    QMailMessageServer(const QMailMessageServer&);
    void operator=(const QMailMessageServer&);

signals:
    void newCountChanged(const QMailMessageCountMap&);

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

public slots:
    void acknowledgeNewMessages(const QMailMessageTypeList& types);

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

    void copyMessages(quint64, const QMailMessageIdList& mailList, const QMailFolderId &destinationId);
    void moveMessages(quint64, const QMailMessageIdList& mailList, const QMailFolderId &destinationId);

    void cancelTransfer(quint64);

    void deleteMessages(quint64, const QMailMessageIdList& mailList, QMailStore::MessageRemovalOption);

    void searchMessages(quint64, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);
    void cancelSearch(quint64);

    void shutdown();

    void protocolRequest(quint64, const QMailAccountId &accountId, const QString &request, const QVariant &data);

private:
    QMailMessageServerPrivate* d;
};

Q_DECLARE_METATYPE(QMailMessageCountMap)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailMessageCountMap, QMailMessageCountMap)

#endif
