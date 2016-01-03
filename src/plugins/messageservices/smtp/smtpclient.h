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

#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <qstring.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qlist.h>
#include <qmailaccountconfiguration.h>
#include <qmailmessage.h>
#include <qmailmessageserver.h>
#include <qmailtransport.h>

QT_BEGIN_NAMESPACE

class QTemporaryFile;

QT_END_NAMESPACE

struct RawEmail
{
    QString from;
    QStringList to;
    QMailMessage mail;
};

class SmtpClient : public QObject
{
    Q_OBJECT

public:
    SmtpClient(QObject* parent);
    ~SmtpClient();

    QMailMessage::MessageType messageType() const;

    void setAccount(const QMailAccountId &accountId);
    QMailAccountId account() const;

    void newConnection();
    void cancelTransfer(QMailServiceAction::Status::ErrorCode code, const QString &text);

    QMailServiceAction::Status::ErrorCode addMail(const QMailMessage& mail);

signals:
    void connectionError(QMailServiceAction::Status::ErrorCode status, const QString &msg);
    void errorOccurred(int, const QString &);
    void errorOccurred(const QMailServiceAction::Status &, const QString &);
    void updateStatus(const QString &);

    void progressChanged(uint, uint);
    void messageTransmitted(const QMailMessageId&);
    void sendCompleted();

protected slots:
    void connected(QMailTransport::EncryptType encryptType);
    void transportError(int, QString msg);
    void readyRead();
    void sent(qint64);
    void accountsUpdated(const QMailAccountIdList&);

private slots:
    void sendMoreData(qint64);
    void authExpired();

private:
    void sendCommand(const char *data, int len = -1, bool maskDebug = false);
    void sendCommand(const QString &cmd, bool maskDebug = false);
    void sendCommand(const QByteArray &cmd, bool maskDebug = false);
    void sendCommands(const QStringList &cmds);
    void incomingData();
    void processResponse(const QString &response);
    void nextAction(const QString &response);
    void messageProcessed(const QMailMessageId &id);

    void operationFailed(int code, const QString &text);
    void operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text);
    void stopTransferring();

private:
    enum TransferStatus
    {
        Init, Helo, Extension, StartTLS, TLS, Connected, Authenticating, Authenticated,
        MetaData, From, Recv, MRcv, PrepareData, Data, Body, Chunk, ChunkSent, Sent, Quit, Done
    };

    QMailAccountConfiguration config;
    TransferStatus status;
    QList<RawEmail> mailList;
    QList<RawEmail>::Iterator mailItr;
    QList<QMailMessage::MessageChunk> mailChunks;
    QMailMessageId sendingId;
    uint messageLength;
    uint sentLength;
    bool sending, success;
    int outstandingResponses;
    QStringList::Iterator it;
    QMailTransport *transport;
    QByteArray lineBuffer;

    // SendMap maps id -> (units) to be sent
    typedef QMap<QMailMessageId, uint> SendMap;
    SendMap sendSize;
    uint progressSendSize;
    uint totalSendSize;

    QStringList capabilities;
    quint32 addressComponent;
    QByteArray domainName;

    QTemporaryFile *temporaryFile;
    qint64 waitingForBytes;
    bool linestart;

    QString bufferedResponse;
    bool notUsingAuth;

    QTimer *authTimeout;
};

#endif
