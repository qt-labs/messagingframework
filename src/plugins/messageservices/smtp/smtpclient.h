/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
    void cancelTransfer();

    QMailServiceAction::Status::ErrorCode addMail(const QMailMessage& mail);

signals:
    void errorOccurred(int, const QString &);
    void errorOccurred(QMailServiceAction::Status::ErrorCode, const QString &);
    void updateStatus(const QString &);

    void progressChanged(uint, uint);
    void messageTransmitted(const QMailMessageId&);
    void sendCompleted();

protected slots:
    void connected(QMailTransport::EncryptType encryptType);
    void transportError(int, QString msg);
    void readyRead();
    void sent(qint64);

private:
    void sendCommand(const char *data, int len = -1);
    void sendCommand(const QString &cmd);
    void sendCommand(const QByteArray &cmd);
    void incomingData();
    void nextAction(const QString &response);
    void messageProcessed(const QMailMessageId &id);

    void operationFailed(int code, const QString &text);
    void operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text);

private:
    enum TransferStatus
    {
        Init, Helo, Extension, StartTLS, TLS, Connected, Auth,
        From, Recv, MRcv, Data, Body, Sent, Done
    };

    QMailAccountConfiguration config;
    TransferStatus status;
    QList<RawEmail> mailList;
    QList<RawEmail>::Iterator mailItr;
    QMailMessageId sendingId;
    uint messageLength;
    uint sentLength;
    bool sending, success;
    QStringList::Iterator it;
    QMailTransport *transport;

    // SendMap maps id -> (units) to be sent
    typedef QMap<QMailMessageId, uint> SendMap;
    SendMap sendSize;
    uint progressSendSize;
    uint totalSendSize;

    QStringList capabilities;
    quint32 addressComponent;
    QByteArray domainName;
};

#endif
