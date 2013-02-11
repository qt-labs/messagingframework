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

#ifndef MESSAGESERVER_H
#define MESSAGESERVER_H

#include <qmailmessageserver.h>
#include <QObject>
#include <QSet>
#include <QSocketNotifier>
#include <qcopadaptor.h>

class ServiceHandler;
class MailMessageClient;
class QDSData;
class QMailMessageMetaData;
class QNetworkState;
class NewCountNotifier;

#if defined(SERVER_AS_DLL)
class QMFUTIL_EXPORT MessageServer : public QObject
#else
class MessageServer : public QObject
#endif
{
    Q_OBJECT

public:
    MessageServer(QObject *parent = 0);
    ~MessageServer();

#if defined(Q_OS_UNIX)
    static void hupSignalHandler(int unused); // Unix SIGHUP signal handler
#endif

signals:
    void messageCountUpdated();
#if defined(Q_OS_UNIX)
public slots:
    void handleSigHup(); // Qt signal handler for UNIX SIGHUP signal.
#endif
    
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

    void cleanupTemporaryMessages();

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
#if defined(Q_OS_UNIX)
    static int sighupFd[2];
    QSocketNotifier *snHup;
#endif

};

#endif
