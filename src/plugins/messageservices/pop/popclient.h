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

#ifndef PopClient_H
#define PopClient_H

#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>
#include <qlist.h>
#include <qtimer.h>
#include <qmailaccountconfiguration.h>
#include <qmailmessage.h>
#include <qmailmessageclassifier.h>
#include <qmailmessageserver.h>
#include <qmailtransport.h>
#include <qmailmessagebuffer.h>

class LongStream;
class QMailTransport;
class QMailAccount;

typedef QMap<QString, QMailMessageId> SelectionMap;

class PopClient : public QObject
{
    Q_OBJECT

public:
    PopClient(QObject* parent);
    ~PopClient();

    QMailMessage::MessageType messageType() const;

    void setAccount(const QMailAccountId &accountId);
    QMailAccountId accountId() const;
    bool synchronizationEnabled(const QMailFolderId &id) const;

    void createTransport();
    void deleteTransport();
    void testConnection();
    void newConnection();
    void closeConnection();
    void setOperation(QMailRetrievalAction::RetrievalSpecification spec);
    bool findInbox();
    void setAdditional(uint _additional = 0);
    void setDeleteOperation();
    void setSelectedMails(const SelectionMap& data);
    void checkForNewMessages();
    void cancelTransfer(QMailServiceAction::Status::ErrorCode code, const QString &text);

    void messageFlushed(QMailMessage &message, bool isComplete);
    void removeAllFromBuffer(QMailMessage *message);

signals:
    void errorOccurred(int, const QString &);
    void errorOccurred(QMailServiceAction::Status::ErrorCode, const QString &);
    void updateStatus(const QString &);

    void messageActionCompleted(const QString &uid);

    void progressChanged(uint, uint);
    void retrievalCompleted();

    void allMessagesReceived();

protected slots:
    void messageBufferFlushed();
    void connected(QMailTransport::EncryptType encryptType);
    void transportError(int, QString msg);

    void connectionInactive();
    void incomingData();

private:
    void deactivateConnection();
    int nextMsgServerPos();
    int msgPosFromUidl(QString uidl);
    uint getSize(int pos);
    void uidlIntegrityCheck();
    void createMail();
    void sendCommand(const char *data, int len = -1);
    void sendCommand(const QString& cmd);
    void sendCommand(const QByteArray& cmd);
    QString readResponse();
    void processResponse(const QString &response);
    void nextAction();
    void retrieveOperationCompleted();
    void messageProcessed(const QString &uid);

    void operationFailed(int code, const QString &text);
    void operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text);

private:
    enum TransferStatus
    {
        Init, CapabilityTest, Capabilities, 
        StartTLS, TLS, Connected, Auth, 
        RequestUids, Uidl, UidList,
        RequestSizes, List, SizeList,
        RequestMessage, Retr, Top, MessageDataRetr, MessageDataTop,
        DeleteMessage, Dele, DeleAfterRetr,
        Done, Quit, Exit
    };

    QMailAccountConfiguration config;
    QMailFolderId folderId;
    QTimer inactiveTimer;
    TransferStatus status;
    int messageCount;
    bool selected;
    bool deleting;
    QString message;
    SelectionMap selectionMap;
    SelectionMap::ConstIterator selectionItr;
    uint mailSize;
    uint headerLimit;
    uint additional;
    bool partialContent;

    QMap<QByteArray, int> serverUidNumber;
    QMap<int, QByteArray> serverUid;
    QMap<int, uint> serverSize;

    QString messageUid;
    QStringList newUids;
    QStringList obsoleteUids;
    LongStream *dataStream;

    QMailTransport *transport;

    QString retrieveUid;

    SelectionMap completionList;

    uint progress;
    uint total;

    // RetrievalMap maps uid -> ((units, bytes) to be retrieved, percentage retrieved)
    typedef QMap<QString, QPair< QPair<uint, uint>, uint> > RetrievalMap;
    RetrievalMap retrievalSize;
    uint progressRetrievalSize;
    uint totalRetrievalSize;

    QMailMessageClassifier classifier;

    QStringList capabilities;
    QList<QByteArray> authCommands;
    QTime lastStatusTimer;
    QVector<QMailMessage*> _bufferedMessages;
    QVector<QMailMessageBufferFlushCallback*> callbacks;
    bool testing;
    bool pendingDeletes;
};

#endif
