/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef MESSAGESERVER_H
#define MESSAGESERVER_H

#include <qmailmessageserver.h>
#include <QObject>
#include <QSet>

class ServiceHandler;
class MailMessageClient;
class QDSData;
class QMailMessageMetaData;
class QNetworkState;
class NewCountNotifier;

class MessageServer : public QObject
{
    Q_OBJECT

public:
    MessageServer(QObject *parent = 0);
    ~MessageServer();

signals:
    void messageCountUpdated();

private slots:
    void retrievalCompleted(quint64 action);

    void transmissionCompleted(quint64 action);

    void response(bool handled);
    void error(const QString &message);

    void messagesAdded(const QMailMessageIdList &ids);
    void messagesUpdated(const QMailMessageIdList &ids);
    void messagesRemoved(const QMailMessageIdList &ids);
    void reportNewCounts();
    void acknowledgeNewMessages(const QMailMessageTypeList&);

private:
    int newMessageCount(QMailMessage::MessageType type) const;

    void updateNewMessageCounts();

    ServiceHandler *handler;
    MailMessageClient *client;
    QMailMessageCountMap messageCounts;

    QCopAdaptor messageCountUpdate;
    QMap<NewCountNotifier*, QMailMessage::MessageType> actionType;
    int newMessageTotal;

    QSet<QMailMessageId> completionList;
    bool completionAttempted;
};

#endif
