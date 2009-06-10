/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
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

#include "smtpclient.h"

#include "smtpauthenticator.h"
#include "smtpconfiguration.h"

#include <QHostAddress>
#include <QTextCodec>
#include <qmaillog.h>
#include <qmailaddress.h>
#include <qmailstore.h>
#include <qmailtransport.h>


static bool initialiseRng()
{
    qsrand(QDateTime::currentDateTime().toUTC().toTime_t());
    return true;
}

static QByteArray messageId(const QByteArray& domainName, quint32 addressComponent)
{
    static bool rngInitialised(initialiseRng());
    Q_UNUSED(rngInitialised)

    quint32 randomComponent(static_cast<quint32>(qrand()));
    quint32 timeComponent(QDateTime::currentDateTime().toUTC().toTime_t());

    return ("<" + 
            QString::number(randomComponent, 36) + 
            "." +
            QString::number(timeComponent, 36) +
            "." +
            QString::number(addressComponent, 36) +
            "-qmf@" +
            domainName +
            ">").toAscii();
}


SmtpClient::SmtpClient(QObject* parent)
    : QObject(parent)
{
    sending = false;
    transport = 0;
}

SmtpClient::~SmtpClient()
{
    delete transport;
}

QMailMessage::MessageType SmtpClient::messageType() const
{
    return QMailMessage::Email;
}

void SmtpClient::setAccount(const QMailAccountId &id)
{
    // Load the current configuration for this account
    config = QMailAccountConfiguration(id);
}

QMailAccountId SmtpClient::account() const
{
    return config.id();
}

void SmtpClient::newConnection()
{
    qMailLog(SMTP) << "newConnection" << flush;
    if (sending) {
        operationFailed(QMailServiceAction::Status::ErrConnectionInUse, tr("Cannot send message; transport in use"));
        return;
    }

    if (!config.id().isValid()) {
        status = Done;
        operationFailed(QMailServiceAction::Status::ErrConfiguration, tr("Cannot send message without account configuration"));
        return;
    }

    SmtpConfiguration smtpCfg(config);
    if ( smtpCfg.smtpServer().isEmpty() ) {
        status = Done;
        operationFailed(QMailServiceAction::Status::ErrConfiguration, tr("Cannot send message without SMTP server configuration"));
        return;
    }

    // Calculate the total indicative size of the messages we're sending
    totalSendSize = 0;
    foreach (uint size, sendSize.values())
        totalSendSize += size;

    progressSendSize = 0;
    emit progressChanged(progressSendSize, totalSendSize);

    status = Init;
    sending = true;
    domainName = QByteArray();

    if (!transport) {
        // Set up the transport
        transport = new QMailTransport("SMTP");

        connect(transport, SIGNAL(readyRead()),
                this, SLOT(readyRead()));
        connect(transport, SIGNAL(connected(QMailTransport::EncryptType)),
                this, SLOT(connected(QMailTransport::EncryptType)));
        connect(transport, SIGNAL(bytesWritten(qint64)),
                this, SLOT(sent(qint64)));
        connect(transport, SIGNAL(updateStatus(QString)),
                this, SIGNAL(updateStatus(QString)));
        connect(transport, SIGNAL(errorOccurred(int,QString)),
                this, SLOT(transportError(int,QString)));
    }

    qMailLog(SMTP) << "Open SMTP connection" << flush;
    transport->open(smtpCfg.smtpServer(), smtpCfg.smtpPort(), static_cast<QMailTransport::EncryptType>(smtpCfg.smtpEncryption()));
}

QMailServiceAction::Status::ErrorCode SmtpClient::addMail(const QMailMessage& mail)
{
    // We shouldn't have anything left in our send list...
    if (mailList.isEmpty() && !sendSize.isEmpty()) {
        foreach (const QMailMessageId& id, sendSize.keys())
            qMailLog(Messaging) << "Message" << id << "still in send map...";

        sendSize.clear();
    }

    if (mail.status() & QMailMessage::HasUnresolvedReferences) {
        qMailLog(Messaging) << "Cannot send SMTP message with unresolved references!";
        return QMailServiceAction::Status::ErrInvalidData;
    }

    if (mail.from().address().isEmpty()) {
        qMailLog(Messaging) << "Cannot send SMTP message with empty from address!";
        return QMailServiceAction::Status::ErrInvalidAddress;
    }

    QStringList sendTo;
    foreach (const QMailAddress& address, mail.recipients()) {
        // Only send to mail addresses
        if (address.isEmailAddress())
            sendTo.append(address.address());
    }
    if (sendTo.isEmpty()) {
        qMailLog(Messaging) << "Cannot send SMTP message with empty recipient address!";
        return QMailServiceAction::Status::ErrInvalidAddress;
    }

    RawEmail rawmail;
    rawmail.from = "<" + mail.from().address() + ">";
    rawmail.to = sendTo;
    rawmail.mail = mail;

    mailList.append(rawmail);
    sendSize.insert(mail.id(), mail.indicativeSize());

    return QMailServiceAction::Status::ErrNoError;
}

void SmtpClient::connected(QMailTransport::EncryptType encryptType)
{
    SmtpConfiguration smtpCfg(config);
    if (smtpCfg.smtpEncryption() == encryptType) {
        qMailLog(SMTP) << "Connected" << flush;
        emit updateStatus(tr("Connected"));
    }

#ifndef QT_NO_OPENSSL
    if ((smtpCfg.smtpEncryption() == QMailTransport::Encrypt_TLS) && (status == TLS)) {
        // We have entered TLS mode - restart the SMTP dialog
        sendCommand("EHLO qtopia-messageserver");
        status = Helo;
    }
#endif
}

void SmtpClient::transportError(int errorCode, QString msg)
{
    operationFailed(errorCode, msg);
}

void SmtpClient::sent(qint64 size)
{
    if (sendingId.isValid() && messageLength) {
        SendMap::const_iterator it = sendSize.find(sendingId);
        if (it != sendSize.end()) {
            sentLength += size;
            uint percentage = qMin<uint>(sentLength * 100 / messageLength, 100);

            // Update the progress figure to count the sent portion of this message
            uint partialSize = (*it) * percentage / 100;
            emit progressChanged(progressSendSize + partialSize, totalSendSize);
        }
    }
}

void SmtpClient::readyRead()
{
    incomingData();
}

void SmtpClient::sendCommand(const char *data, int len)
{
    if (len == -1)
        len = ::strlen(data);

    QDataStream &out(transport->stream());
    out.writeRawData(data, len);
    out.writeRawData("\r\n", 2);

    if (len) {
        qMailLog(SMTP) << "SEND:" << data;
    }
}

void SmtpClient::sendCommand(const QString &cmd)
{
    sendCommand(cmd.toAscii());
}

void SmtpClient::sendCommand(const QByteArray &cmd)
{
    sendCommand(cmd.data(), cmd.length());
}

void SmtpClient::incomingData()
{
    while (transport->canReadLine()) {
        QString response = transport->readLine();
        qMailLog(SMTP) << "RECV:" << response.left(response.length() - 2) << flush;
        nextAction(response);
    }
}

void SmtpClient::nextAction(const QString &response)
{
    uint responseCode(0);
    if (!response.isEmpty())
        responseCode = response.left(3).toUInt();

    switch (status) {
    case Init:  
    {
        if (responseCode == 220) {
            mailItr = mailList.begin();
            capabilities.clear();

            // We need to know if extensions are supported
            sendCommand("EHLO qtopia-messageserver");
            status = Helo;
        } else {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        } 
        break;
    }
    case Helo:
    {
        if (responseCode == 500) {
            // EHLO is not implemented by this server - fallback to HELO
            sendCommand("HELO qtopia-messageserver");
        } else if (responseCode == 250) {
            if (domainName.isEmpty()) {
                // Extract the domain name from the greeting
                int index = response.indexOf(' ', 4);
                if (index == -1) {
                    domainName = response.mid(4).trimmed().toAscii();
                } else {
                    domainName = response.mid(4, index - 4).trimmed().toAscii();
                }
            }

            if (response[3] == '-') {
                // This is to be followed by extension keywords
                status = Extension;
            } else {
                // No extension data available - proceed to TLS negotiation
                status = StartTLS;
                nextAction(QString());
            }
        } else {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }
    case Extension:
    {
        if (responseCode == 250) {
            capabilities.append(response.mid(4).trimmed());

            if (response[3] == '-') {
                // More to follow
            } else {
                // This is the terminating extension
                // Now that we know the capabilities, check for Reference support
                bool supportsBURL(false);
                bool supportsChunking(false);
                foreach (const QString &capability, capabilities) {
                    // Although technically only BURL is needed for Reference support, CHUNKING
                    // is also necessary to allow any additional data to accompany the reference
                    if ((capability == "BURL") || (capability.startsWith("BURL "))) {
                        supportsBURL = true;
                    } else if (capability == "CHUNKING") {
                        supportsChunking = true;
                    }
                }

                const bool supportsReferences(supportsBURL && supportsChunking);

                QMailAccount account(config.id());
                if (((account.status() & QMailAccount::CanTransmitViaReference) && !supportsReferences) ||
                    (!(account.status() & QMailAccount::CanTransmitViaReference) && supportsReferences)) {
                    account.setStatus(QMailAccount::CanTransmitViaReference, supportsReferences);
                    if (!QMailStore::instance()->updateAccount(&account)) {
                        qWarning() << "Unable to update account" << account.id() << "to set CanTransmitViaReference";
                    }
                }

                // Proceed to TLS negotiation
                status = StartTLS;
                nextAction(QString());
            }
        } else {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        } 
        break;
    }
    case StartTLS:
    {
#ifndef QT_NO_OPENSSL
        SmtpConfiguration smtpCfg(config);
        const bool useTLS(smtpCfg.smtpEncryption() == QMailTransport::Encrypt_TLS);
#else
        const bool useTLS(false);
#endif

        if (useTLS && !transport->isEncrypted()) {
            sendCommand("STARTTLS");
            status = TLS;
        } else {
            status = Connected;
            nextAction(QString());
        }
        break;
    }
    case TLS:
    {
        if (responseCode == 220) {
#ifndef QT_NO_OPENSSL
            // Switch into encrypted mode
            transport->switchToEncrypted();
#endif
        } else  {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }

    case Connected:
    {
        // We are now connected with appropriate encryption
        
        // Find the properties of our connection
        QHostAddress localAddress(transport->socket().localAddress());
        if (localAddress.isNull()) {
            // Better to use the remote address than nothing...
            localAddress = transport->socket().peerAddress();
            if (localAddress.isNull()) {
                localAddress = QHostAddress(QHostAddress::LocalHost);
            }
        }
        addressComponent = localAddress.toIPv4Address();

        // Find the authentication mode to use
        QByteArray authCmd(SmtpAuthenticator::getAuthentication(config.serviceConfiguration("smtp"), capabilities));
        if (!authCmd.isEmpty()) {
            sendCommand(authCmd);
            status = Authenticating;
        } else {
            status = Authenticated;
            nextAction(QString());
        }
        break;
    }
    case Authenticating:
    {
        if (responseCode == 334) {
            // This is a continuation containing a challenge string (in Base64)
            QByteArray challenge = QByteArray::fromBase64(response.mid(4).toAscii());
            QByteArray response(SmtpAuthenticator::getResponse(config.serviceConfiguration("smtp"), challenge));

            if (!response.isEmpty()) {
                // Send the response as Base64 encoded
                sendCommand(response.toBase64());
                return;
            }
        } else if (responseCode == 235) {
            // We are now authenticated
            status = Authenticated;
            nextAction(QString());
        } else {
            operationFailed(QMailServiceAction::Status::ErrLoginFailed, response);
        }

        // Otherwise, we're authenticated
        break;
    }
    case Authenticated:
    {
        if (mailItr == mailList.end()) {
            // Nothing to send
            status = Quit;
        } else {
            status = From;
        }
        nextAction(QString());
        break;
    }

    case From:  
    {
        sendCommand("MAIL FROM: " + mailItr->from);
        status = Recv;

        emit updateStatus(tr( "Sending: %1").arg(mailItr->mail.subject()) );
        break;
    }
    case Recv:  
    {
        if (responseCode == 250) {
            it = mailItr->to.begin();
            if (it == mailItr->to.end()) {
                operationFailed(QMailServiceAction::Status::ErrInvalidAddress, "no recipients");
            } else {
                sendCommand("RCPT TO: <" + *it + ">");
                status = MRcv;
            }
        } else  {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        } 
        break;
    }
    case MRcv:  
    {
        if ((responseCode == 250) || (responseCode == 251)) {
            it++;
            if ( it != mailItr->to.end() ) {
                sendCommand("RCPT TO: <" + *it + ">");
            } else  {
                if (mailItr->mail.status() & QMailMessage::HasReferences) {
                    mailChunks = mailItr->mail.toRfc2822Chunks(QMailMessage::TransmissionFormat);
                    if (mailChunks.isEmpty()) {
                        // Nothing to send?
                        status = Sent;
                    } else {
                        status = Chunk;
                    }
                } else {
                    status = Data;
                }
                nextAction(QString());
            }
        } else {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }

    case Data:  
    {
        sendCommand("DATA");
        status = Body;
        break;
    }
    case Body:  
    {
        if (responseCode == 354) {
            sendingId = mailItr->mail.id();
            sentLength = 0;

            // Set the message's message ID
            mailItr->mail.setHeaderField("Message-ID", messageId(domainName, addressComponent));

            transport->mark();
            mailItr->mail.toRfc2822(transport->stream(), QMailMessage::TransmissionFormat);
            messageLength = transport->bytesSinceMark();

            transport->stream().writeRawData("\r\n.\r\n", 5);

            qMailLog(SMTP) << "Body: sent:" << messageLength << "bytes";

            status = Sent;
        } else {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }

    case Chunk:
    {
        const bool isLast(mailChunks.count() == 1);
        QMailMessage::MessageChunk chunk(mailChunks.takeFirst());

        QByteArray command;
        if (chunk.first == QMailMessage::Text) {
            sendCommand("BDAT " + QByteArray::number(chunk.second.size()) + (isLast ? " LAST" : ""));

            // The data follows immediately
            transport->stream().writeRawData(chunk.second.constData(), chunk.second.size());
        } else if (chunk.first == QMailMessage::Reference) {
            sendCommand("BURL " + chunk.second + (isLast ? " LAST" : ""));
        }

        status = (isLast ? Sent : ChunkSent);
        break;
    }
    case ChunkSent:
    {
        if (responseCode == 250) {
            // Move on to the next chunk
            status = Chunk;
            nextAction(QString());
        } else {
            QMailMessageId msgId(mailItr->mail.id());

            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
            messageProcessed(msgId);
        }
        break;
    }

    case Sent:  
    {
        QMailMessageId msgId(mailItr->mail.id());

        // The last send operation is complete
        if (responseCode == 250) {
            if (msgId.isValid()) {
                // Update the message to store the message-ID and mark as sent
                mailItr->mail.setStatus(Sent, true);
                if (!QMailStore::instance()->updateMessage(&mailItr->mail)) {
                    qWarning() << "Unable to update message after transmission:" << msgId;
                }

                emit messageTransmitted(msgId);

                messageProcessed(msgId);
                sendingId = QMailMessageId();
            }

            mailItr++;
            if (mailItr == mailList.end()) {
                status = Quit;
            } else {
                // More messages to send
                status = From;
            }
            nextAction(QString());
        } else {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
            messageProcessed(msgId);
        }
        break;
    }

    case Quit:  
    {
        // Completed successfully
        sendCommand("QUIT");

        sending = false;
        status = Done;
        transport->close();
        qMailLog(SMTP) << "Closed connection" << flush;

        int count = mailList.count();
        if (count) {
            mailList.clear();
            emit updateStatus(tr("Sent %n messages", "", count));
        } else {
            // There was nothing to send
            emit sendCompleted();
        }
        break;
    }

    case Done:  
    {
        // Supposed to be unused here
        qWarning() << "nextAction - Unexpected status value: " << status;
        break;
    }

    }
}

void SmtpClient::cancelTransfer()
{
    operationFailed(QMailServiceAction::Status::ErrCancel, tr("Cancelled by user"));
}

void SmtpClient::messageProcessed(const QMailMessageId &id)
{
    SendMap::iterator it = sendSize.find(id);
    if (it != sendSize.end()) {
        // Update the progress figure
        progressSendSize += *it;
        emit progressChanged(progressSendSize, totalSendSize);

        sendSize.erase(it);
        if (sendSize.isEmpty()) {
            // We have attempted to send all the supplied messages
            emit sendCompleted();
        }
    }
}

void SmtpClient::operationFailed(int code, const QString &text)
{
    if (sending) {
        transport->close();
        qMailLog(SMTP) << "Closed connection:" << text << flush;
        
        sendingId = QMailMessageId();
        sending = false;
        mailList.clear();
        sendSize.clear();
    }

    emit errorOccurred(code, text);
}

void SmtpClient::operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    if (sending) {
        transport->close();
        qMailLog(SMTP) << "Closed connection:" << text << flush;
        
        sendingId = QMailMessageId();
        sending = false;
        mailList.clear();
        sendSize.clear();
    }

    QString msg;
    if (code == QMailServiceAction::Status::ErrUnknownResponse) {
        if (config.id().isValid()) {
            SmtpConfiguration smtpCfg(config);
            msg = smtpCfg.smtpServer() + ": ";
        }
    }
    msg.append(text);

    emit errorOccurred(code, msg);
}

