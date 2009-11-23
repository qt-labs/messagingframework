/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
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
