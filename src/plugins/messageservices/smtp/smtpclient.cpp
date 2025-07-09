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

#include "smtpclient.h"

#include "smtpauthenticator.h"
#include "smtpconfiguration.h"

#include <QHostAddress>
#include <QTextCodec>
#include <QTemporaryFile>
#include <QCoreApplication>
#include <QDir>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QRegExp>
#ifndef QT_NO_SSL
#include <QSslSocket>
#endif
#include <qmailaddress.h>
#include <qmailstore.h>
#include <qmailtransport.h>
#include <qmailnamespace.h>
#include <qmailauthenticator.h>

Q_LOGGING_CATEGORY(lcSMTP, "org.qt.messageserver.smtp", QtWarningMsg)

// The size of the buffer used when sending messages.
// Only this many bytes is queued to be sent at a time.
#define SENDING_BUFFER_SIZE 5000

static QByteArray messageId(const QByteArray& domainName, quint32 addressComponent)
{
    quint32 randomComponent(QRandomGenerator::global()->generate());
    quint32 timeComponent(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000);

    return ('<' +
            QString::number(randomComponent, 36) +
            '.' +
            QString::number(timeComponent, 36) +
            '.' +
            QString::number(addressComponent, 36) +
            "-qmf@" +
            domainName +
            '>').toLatin1();
}

static QByteArray localName(const QHostAddress &hostAddress)
{

    QByteArray result(QHostInfo::localDomainName().toLatin1());
    if (!result.isEmpty())
        return result;
    if (hostAddress.protocol() == QAbstractSocket::IPv6Protocol)
        return "[IPv6:" + hostAddress.toString().toLatin1() + "]";
    else if (!hostAddress.isNull())
        return "[" + hostAddress.toString().toLatin1() + "]";
    QList<QHostAddress> addresses(QNetworkInterface::allAddresses());
    if (addresses.isEmpty())
        return "localhost.localdomain";
    QHostAddress addr;
    // try to find a non-loopback address
    foreach (const QHostAddress &a, addresses) {
        if (!a.isLoopback() && !a.isNull()) {
            addr = a;
            break;
        }
    }
    if (addr.isNull())
        addr = addresses.first();

    return "[" + addr.toString().toLatin1() + "]";
}

SmtpClient::SmtpClient(const QMailAccountId &id, QObject* parent)
    : QObject(parent)
    , config(QMailAccountConfiguration(id))
    , mailItr(mailList.end())
    , messageLength(0)
    , fetchingCapabilities(false)
    , transport(0)
    , temporaryFile(0)
    , waitingForBytes(0)
    , notUsingAuth(false)
    , authReset(false)
    , authTimeout(0)
    , credentials(QMailCredentialsFactory::getCredentialsHandlerForAccount(config))
{
}

SmtpClient::~SmtpClient()
{
    delete transport;
    delete temporaryFile;
    delete authTimeout;
    delete credentials;
}

QMailAccountId SmtpClient::account() const
{
    return config.id();
}

void SmtpClient::fetchCapabilities()
{
    qCDebug(lcSMTP) << "fetchCapabilities";
    capabilities.clear();

    if (transport && transport->inUse()) {
        qCWarning(lcSMTP) << "Cannot fetch capabilities; transport in use";
        emit fetchCapabilitiesFinished();
        return;
    }

    if (!account().isValid()) {
        qCWarning(lcSMTP) << "Cannot fetch capabilities; invalid account";
        emit fetchCapabilitiesFinished();
        return;
    }

    // Reload the account configuration whenever a new SMTP
    // connection is created, in order to ensure the changes
    // in the account settings are being managed properly.
    config = QMailAccountConfiguration(account());
    SmtpConfiguration smtpConfig(config);
    if (smtpConfig.smtpServer().isEmpty()) {
        qCWarning(lcSMTP) << "Cannot fetch capabilities without SMTP server configuration";
        emit fetchCapabilitiesFinished();
        return;
    }

    fetchingCapabilities = true;
    openTransport();
}

void SmtpClient::openTransport()
{
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
#ifndef QT_NO_SSL
        connect(transport, SIGNAL(sslErrorOccured(QMailServiceAction::Status::ErrorCode,QString)),
                this, SIGNAL(connectionError(QMailServiceAction::Status::ErrorCode,QString)));
#endif
    }

    status = Init;
    outstandingResponses = 0;

    qCDebug(lcSMTP) << "Open SMTP connection";
    SmtpConfiguration smtpConfig(config);
    transport->setAcceptUntrustedCertificates(smtpConfig.acceptUntrustedCertificates());
    transport->open(smtpConfig.smtpServer(), smtpConfig.smtpPort(),
                    static_cast<QMailTransport::EncryptType>(smtpConfig.smtpEncryption()));
}

void SmtpClient::newConnection()
{
    qCDebug(lcSMTP) << "newConnection";

    if (transport && transport->inUse()) {
        operationFailed(QMailServiceAction::Status::ErrConnectionInUse, tr("Cannot send message; transport in use"));
        return;
    }

    if (!account().isValid()) {
        status = Done;
        operationFailed(QMailServiceAction::Status::ErrConfiguration, tr("Cannot send message without account configuration"));
        return;
    }

    // Reload the account configuration whenever a new SMTP
    // connection is created, in order to ensure the changes
    // in the account settings are being managed properly.
    config = QMailAccountConfiguration(account());
    SmtpConfiguration smtpConfig(config);
    if (smtpConfig.smtpServer().isEmpty()) {
        status = Done;
        operationFailed(QMailServiceAction::Status::ErrConfiguration, tr("Cannot send message without SMTP server configuration"));
        return;
    }

    if (credentials) {
        credentials->init(smtpConfig);
    }

    // Calculate the total indicative size of the messages we're sending
    totalSendSize = 0;
    foreach (uint size, sendSize.values())
        totalSendSize += size;

    progressSendSize = 0;
    emit progressChanged(progressSendSize, totalSendSize);

    fetchingCapabilities = false;
    domainName = QByteArray();
    authReset = false;

    openTransport();
}

QMailServiceAction::Status::ErrorCode SmtpClient::addMail(const QMailMessage& mail)
{
    // We shouldn't have anything left in our send list...
    if (mailList.isEmpty() && !sendSize.isEmpty()) {
        foreach (const QMailMessageId& id, sendSize.keys())
            qCWarning(lcSMTP) << "Message" << id << "still in send map...";

        sendSize.clear();
    }

    if (mail.status() & QMailMessage::HasUnresolvedReferences) {
        qCWarning(lcSMTP) << "Cannot send SMTP message with unresolved references!";
        return QMailServiceAction::Status::ErrInvalidData;
    }

    if (mail.from().address().isEmpty()) {
        qCWarning(lcSMTP) << "Cannot send SMTP message with empty from address!";
        return QMailServiceAction::Status::ErrInvalidAddress;
    }

    QStringList sendTo;
    foreach (const QMailAddress& address, mail.recipients()) {
        // Only send to mail addresses
        if (address.isEmailAddress())
            sendTo.append(address.address());
    }
    if (sendTo.isEmpty()) {
        qCWarning(lcSMTP) << "Cannot send SMTP message with empty recipient address!";
        return QMailServiceAction::Status::ErrInvalidAddress;
    }

    RawEmail rawmail;
    rawmail.from = "<" + mail.from().address() + ">";
    rawmail.to = sendTo;
    rawmail.mail = mail;

    mailList.append(rawmail);
    mailItr = mailList.end();
    sendSize.insert(mail.id(), mail.indicativeSize());

    return QMailServiceAction::Status::ErrNoError;
}

void SmtpClient::connected(QMailTransport::EncryptType encryptType)
{
    delete authTimeout;
    authTimeout = new QTimer;
    authTimeout->setSingleShot(true);
    const int twentySeconds = 40 * 1000;
    connect(authTimeout, SIGNAL(timeout()), this, SLOT(authExpired()));
    authTimeout->setInterval(twentySeconds);
    authTimeout->start();

    SmtpConfiguration smtpCfg(config);
    if (smtpCfg.smtpEncryption() == encryptType) {
        qCDebug(lcSMTP) << "Connected";
        emit updateStatus(tr("Connected"));
    }

#ifndef QT_NO_SSL
    if ((smtpCfg.smtpEncryption() == QMailTransport::Encrypt_TLS) && (status == TLS)) {
        // We have entered TLS mode - restart the SMTP dialog
        QByteArray ehlo("EHLO " + localName(transport->socket().localAddress()));
        sendCommand(ehlo);
        status = Helo;
    }
#endif
}

void SmtpClient::transportError(int errorCode, QString msg)
{
    if (status == Done)
        return; //Ignore errors after QUIT is sent

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

void SmtpClient::sendCommand(const char *data, int len, bool maskDebug)
{
    if (len == -1)
        len = ::strlen(data);

    QDataStream &out(transport->stream());
    out.writeRawData(data, len);
    out.writeRawData("\r\n", 2);

    ++outstandingResponses;

    if (maskDebug) {
        qCDebug(lcSMTP) << "SEND: <login hidden>";
    } else {
        QString logCmd = QString::fromLatin1(data);
        QRegExp loginExp("^AUTH\\s[^\\s]+\\s");
        if (loginExp.indexIn(data) != -1) {
            logCmd = logCmd.left(loginExp.matchedLength()) + "<login hidden>";
        }

        qCDebug(lcSMTP) << "SEND:" << logCmd;
    }
}

void SmtpClient::sendCommand(const QString &cmd, bool maskDebug)
{
    sendCommand(cmd.toLatin1(), maskDebug);
}

void SmtpClient::sendCommand(const QByteArray &cmd, bool maskDebug)
{
    sendCommand(cmd.data(), cmd.length(), maskDebug);
}

void SmtpClient::sendCommands(const QStringList &cmds)
{
    foreach (const QString &cmd, cmds)
        sendCommand(cmd.toLatin1());
}

void SmtpClient::incomingData()
{
    if (!lineBuffer.isEmpty() && transport->canReadLine()) {
        processResponse(QString::fromLatin1(lineBuffer + transport->readLine()));
        lineBuffer.clear();
    }

    while (transport->canReadLine()) {
        processResponse(QString::fromLatin1(transport->readLine()));
    }

    if (transport->bytesAvailable()) {
        // If there is an incomplete line, read it from the socket buffer to ensure we get readyRead signal next time
        lineBuffer.append(transport->readAll());
    }
}

void SmtpClient::processResponse(const QString &response)
{
    qCDebug(lcSMTP) << "RECV:" << response.left(response.length() - 2);

    delete authTimeout;
    authTimeout = 0;

    if (notUsingAuth) {
        if (response.startsWith("530")) {
            operationFailed(QMailServiceAction::Status::ErrConfiguration, response);
            return;
        } else {
            notUsingAuth = false;
        }
    }

    if (outstandingResponses > 0) {
        --outstandingResponses;
    }

    if (outstandingResponses > 0) {
        // For pipelined commands, just ensure that they did not fail
        if (!response.isEmpty() && (response[0] != QChar('2'))) {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
    } else {
        nextAction(response);
    }
}

void SmtpClient::nextAction(const QString &response)
{
    uint responseCode(0);
    if (!response.isEmpty())
        responseCode = response.left(3).toUInt();

    // handle multi-line response, but 250 is a special case
    if ((responseCode != 250) && (response.length() >= 4) && (response[3] == '-')) {
        bufferedResponse += response.mid(4).trimmed();
        bufferedResponse += ' ';
        return;
    }

    switch (status) {
    case Init:
    {
        if (responseCode == 220) {
            mailItr = mailList.begin();
            capabilities.clear();

            // We need to know if extensions are supported
            QByteArray ehlo("EHLO " + localName(transport->socket().localAddress()));
            sendCommand(ehlo);
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
            QByteArray ehlo("HELO " + localName(transport->socket().localAddress()));
            sendCommand(ehlo);
        } else if (responseCode == 250) {
            if (domainName.isEmpty()) {
                // Extract the domain name from the greeting
                int index = response.indexOf(' ', 4);
                if (index == -1) {
                    domainName = response.mid(4).trimmed().toLatin1();
                } else {
                    domainName = response.mid(4, index - 4).trimmed().toLatin1();
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
                    (!(account.status() & QMailAccount::CanTransmitViaReference) && supportsReferences) ||
                    (account.customField("qmf-smtp-capabilities-listed") != "true")) {
                    account.setCustomField("qmf-smtp-capabilities-listed", "true");
                    account.setStatus(QMailAccount::CanTransmitViaReference, supportsReferences);
                    if (!QMailStore::instance()->updateAccount(&account)) {
                        qCWarning(lcSMTP) << "Unable to update account" << account.id() << "to set CanTransmitViaReference";
                    }
                }

                // Available authentication mechanisms
                SmtpConfigurationEditor smtpCfg(&config);
                if (smtpCfg.smtpAuthentication() == QMail::NoMechanism
                    && smtpCfg.smtpAuthFromCapabilities()) {
                    QStringList authCaps;
                    foreach (QString const& capability, capabilities) {
                        if (capability.startsWith("AUTH", Qt::CaseInsensitive)) {
                            authCaps.append(capability.split(" ", Qt::SkipEmptyParts));
                        }
                    }
                    QMail::SaslMechanism authType = QMailAuthenticator::authFromCapabilities(authCaps);
                    if (authType != QMail::NoMechanism) {
                        smtpCfg.setSmtpAuthentication(authType);
                        if (!QMailStore::instance()->updateAccountConfiguration(&config)) {
                            qCWarning(lcSMTP) << "Unable to update account" << config.id()
                                       << "with auth type" << authType;
                        }
                    }
                }

                if (fetchingCapabilities) {
                    status = Done;
                    transport->close();
                    qCDebug(lcSMTP) << "Closed connection";
                    emit fetchCapabilitiesFinished();
                } else {
                    // Proceed to TLS negotiation
                    status = StartTLS;
                    nextAction(QString());
                }
            }
        } else {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }
    case StartTLS:
    {
#ifndef QT_NO_SSL
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
#ifndef QT_NO_SSL
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

        status = Authenticate;
        nextAction(QString());
        break;
    }
    case Authenticate:
    {
        // Find the authentication mode to use
        authCommands = SmtpAuthenticator::getAuthentication(SmtpConfiguration(config), *credentials);
        if (authCommands.isEmpty()) {
            // No auth in use.
            foreach (QString const& capability, capabilities) {
                if (capability.startsWith("AUTH", Qt::CaseInsensitive)) {
                    notUsingAuth = true;
                    break;
                }
            }
            status = Authenticated;
            nextAction(QString());
        } else if (credentials->status() == QMailCredentialsInterface::Fetching) {
            connect(credentials, &QMailCredentialsInterface::statusChanged,
                    this, &SmtpClient::onCredentialsStatusChanged);
        } else if (credentials->status() == QMailCredentialsInterface::Failed) {
            operationFailed(QMailServiceAction::Status::ErrConfiguration,
                            credentials->lastError());
        } else {
            // Send auth commands one by one.
            sendCommand(authCommands.takeFirst());
            status = Authenticating;
        }
        break;
    }
    case Authenticating:
    {
        if (responseCode == 334 && !authCommands.isEmpty()) {
            // Continue to send auth commands one by one.
            sendCommand(authCommands.takeFirst(), true);
        } else if (responseCode == 334) {
            // This is a continuation containing a challenge string (in Base64)
            QByteArray challenge = QByteArray::fromBase64(response.mid(4).toLatin1());
            QByteArray response(SmtpAuthenticator::getResponse(SmtpConfiguration(config), challenge, *credentials));

            if (!response.isEmpty()) {
                // Send the response as Base64 encoded, mask the debug output
                sendCommand(response.toBase64(), true);
                bufferedResponse.clear();
                return;
            } else {
                // Challenge response is empty
                // send a empty response.
                sendCommand("");
            }
        } else if (responseCode == 235) {
            // We are now authenticated
            status = Authenticated;
            nextAction(QString());
        } else if (responseCode == 504) {
            SmtpConfiguration smtpCfg(config);
            // reset method used and try again to authenticated from caps
            if (smtpCfg.smtpAuthFromCapabilities() && !authReset) {
                qCDebug(lcSMTP) << "Resetting AUTH TYPE";
                authReset = true;
                QMailAccountConfiguration accountConfig(smtpCfg.id());
                SmtpConfigurationEditor smtpCfgEditor(&accountConfig);
                smtpCfgEditor.setSmtpAuthentication(QMail::NoMechanism);
                if (!QMailStore::instance()->updateAccount(nullptr, &accountConfig)) {
                    qCWarning(lcSMTP) << "Unable to update account" << smtpCfg.id()
                               << "auth type.";
                    operationFailed(QMailServiceAction::Status::ErrConfiguration, response);
                }
                // Restart the authentication process
                QByteArray ehlo("EHLO " + localName(transport->socket().localAddress()));
                sendCommand(ehlo);
                status = Helo;
            } else {
                operationFailed(QMailServiceAction::Status::ErrConfiguration, response);
            }
        } else if (responseCode == 530) {
            operationFailed(QMailServiceAction::Status::ErrConfiguration, response);
        } else if (responseCode == 535) {
            if (!authReset) {
                authReset = true;
                // Credentials may have changed, fetch them again.
                if (credentials->init(SmtpConfiguration(config))) {
                    status = Connected;
                    nextAction(QString());
                } else {
                    operationFailed(QMailServiceAction::Status::ErrConfiguration,
                                    credentials->lastError());
                }
            } else {
                credentials->invalidate(QStringLiteral("messageserver5"));
                operationFailed(QMailServiceAction::Status::ErrLoginFailed, response);
            }
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
            status = MetaData;
        }
        nextAction(QString());
        break;
    }

    case MetaData:
    {
        if (capabilities.contains("PIPELINING")) {
            // We can send all our non-message commands together
            QStringList commands;
            commands.append("MAIL FROM:" + mailItr->from);
            for (it = mailItr->to.begin(); it != mailItr->to.end(); ++it) {
                commands.append("RCPT TO:<" + *it + ">");
            }
            sendCommands(commands);

            // Continue on with the message data proper
            status = PrepareData;
            nextAction(QString());
        } else {
            // Send meta data elements individually
            status = From;
            nextAction(QString());
        }
        break;
    }
    case From:
    {
        sendCommand("MAIL FROM:" + mailItr->from);
        status = Recv;

        emit updateStatus(tr( "Sending: %1").arg(mailItr->mail.subject().simplified()) );
        break;
    }
    case Recv:
    {
        if (responseCode == 250) {
            it = mailItr->to.begin();
            if (it == mailItr->to.end()) {
                operationFailed(QMailServiceAction::Status::ErrInvalidAddress, "no recipients");
            } else {
                sendCommand("RCPT TO:<" + *it + ">");
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
                sendCommand("RCPT TO:<" + *it + ">");
            } else {
                status = PrepareData;
                nextAction(QString());
            }
        } else {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }

    case PrepareData:
    {
        if (mailItr->mail.status() & QMailMessage::TransmitFromExternal) {
            // We can replace this entire message by a reference to its external location
            mailChunks.append(qMakePair(QMailMessage::Reference, mailItr->mail.externalLocationReference().toLatin1()));
            status = Chunk;
        } else if (mailItr->mail.status() & QMailMessage::HasReferences) {
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

            if (temporaryFile) {
                operationFailed(QMailServiceAction::Status::ErrInvalidData, tr("Received response 354 while sending."));
                break;
            }

            linestart = true;
            sendingId = mailItr->mail.id();
            sentLength = 0;

            // Set the message's message ID
            mailItr->mail.setHeaderField("Message-ID", messageId(domainName, addressComponent));

            // Buffer the message to a temporary file.
            QString tempPath = QMail::tempPath();
            QDir dir;
            if (!dir.exists(tempPath))
                dir.mkpath(tempPath);
            temporaryFile = new QTemporaryFile(tempPath + QLatin1String("smtptmp.XXXXXX"));
            bool ok = temporaryFile->open();
            Q_ASSERT(ok);
            {
                // Note that there is no progress update while the message is streamed to the file.
                // This isn't optimal but it's how the original code worked and fixing it requires
                // putting this call onto a separate thread because it blocks until done.
                QDataStream dataStream(temporaryFile);
                mailItr->mail.toRfc2822(dataStream, QMailMessage::TransmissionFormat);
            }
            messageLength = temporaryFile->size();
            //qCDebug(lcSMTP) << "Body: queued" << messageLength << "bytes to" << temporaryFile->fileName();

            // Now write the message to the transport in blocks, waiting for the bytes to be
            // written each time so there is no need to allocate large buffers to hold everything.
            temporaryFile->seek(0);
            waitingForBytes = 0;
            if (transport->isEncrypted())
                connect(&(transport->socket()), SIGNAL(encryptedBytesWritten(qint64)), this, SLOT(sendMoreData(qint64)));
            else
                connect(transport, SIGNAL(bytesWritten(qint64)), this, SLOT(sendMoreData(qint64)));

            // trigger the sending of the first block of data
            sendMoreData(0);
        } else {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }

    case Chunk:
    {
        const bool pipelining(capabilities.contains("PIPELINING"));

        do {
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
        } while (pipelining && !mailChunks.isEmpty());

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
                // Update the message to store the message-ID
                if (!QMailStore::instance()->updateMessage(&mailItr->mail)) {
                    qCWarning(lcSMTP) << "Unable to update message with Message-ID:" << msgId;
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
                status = MetaData;
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

        status = Done;
        transport->close();
        qCDebug(lcSMTP) << "Closed connection";

        int count = mailList.count();
        if (count) {
            mailList.clear();
            mailItr = mailList.end();
            emit updateStatus(tr("Sent %n messages", "", count));
        }
        emit sendCompleted();
        break;
    }

    case Done:
    {
        // Supposed to be unused here
        qCWarning(lcSMTP) << "nextAction - Unexpected status value: " << status;
        break;
    }

    }
    bufferedResponse.clear();
}

void SmtpClient::cancelTransfer(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    operationFailed(code, text);
}

void SmtpClient::messageProcessed(const QMailMessageId &id)
{
    SendMap::iterator it = sendSize.find(id);
    if (it != sendSize.end()) {
        // Update the progress figure
        progressSendSize += *it;
        emit progressChanged(progressSendSize, totalSendSize);

        sendSize.erase(it);
    }
}

void SmtpClient::operationFailed(int code, const QString &text)
{
    if (code != QMailServiceAction::Status::ErrNoError) {
        delete authTimeout;
        authTimeout = 0;
    }

    if (transport && transport->inUse()) {
        stopTransferring();
        transport->close();
        qCDebug(lcSMTP) << "Closed connection:" << text;
    }

    if (fetchingCapabilities) {
        emit fetchCapabilitiesFinished();
    } else {
        sendingId = QMailMessageId();
        mailList.clear();
        mailItr = mailList.end();
        sendSize.clear();
        emit errorOccurred(code, bufferedResponse + text);
    }
}

void SmtpClient::operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    if (code != QMailServiceAction::Status::ErrNoError) {
        delete authTimeout;
        authTimeout = 0;
    }

    if (transport && transport->inUse()) {
        stopTransferring();
        transport->close();
        qCDebug(lcSMTP) << "Closed connection:" << text;
    }

    if (fetchingCapabilities) {
        emit fetchCapabilitiesFinished();
    } else {
        QMailServiceAction::Status actionStatus;
        if (sendingId != QMailMessageId()) {
            actionStatus.messageId = sendingId;
        } else if (mailItr != mailList.end()) {
            actionStatus.messageId = mailItr->mail.id();
        }
        actionStatus.errorCode = code;

        sendingId = QMailMessageId();
        mailList.clear();
        mailItr = mailList.end();
        sendSize.clear();

        QString msg;
        if (code == QMailServiceAction::Status::ErrUnknownResponse) {
            if (config.id().isValid()) {
                SmtpConfiguration smtpCfg(config);
                msg = smtpCfg.smtpServer() + ": ";
            }
        }
        msg.append(bufferedResponse);
        msg.append(text);
        emit errorOccurred(actionStatus, msg);
    }
}

void SmtpClient::sendMoreData(qint64 bytesWritten)
{
    Q_ASSERT(status == Body && temporaryFile);

    // Check if we have any pending data still waiting to be sent.
#ifndef QT_NO_SSL
    Q_UNUSED(bytesWritten)
    QSslSocket *socket = qobject_cast<QSslSocket*>(&(transport->socket()));
    Q_ASSERT(socket);
    if (socket->encryptedBytesToWrite()
        || socket->bytesToWrite()) {
        // There is still pending data to be written.
        return;
    }
#else
    waitingForBytes -= bytesWritten;

    // If anyone else writes bytes we end up with a negative value... just reset to 0 when that happens.
    if (waitingForBytes < 0) waitingForBytes = 0;

    // Don't send more data until all bytes have been written.
    if (waitingForBytes) return;
#endif

    // No more data to send
    if (temporaryFile->atEnd()) {
        stopTransferring();
        qCDebug(lcSMTP) << "Body: sent:" << messageLength << "bytes";
        transport->stream().writeRawData("\r\n.\r\n", 5);
        return;
    }

    // Queue up to SENDING_BUFFER_SIZE bytes for transmission
    char buffer[SENDING_BUFFER_SIZE];
    qint64 bytes = temporaryFile->read(buffer, SENDING_BUFFER_SIZE);

    QByteArray dotstuffed;
    dotstuffed.reserve(SENDING_BUFFER_SIZE + 10); // more than 10 stuffs and array may be autoresized
    for (int i = 0; i < bytes; ++i) {
        if (linestart && (buffer[i] == '.')) {
            dotstuffed.append("..");
            linestart = false;
        } else if (buffer[i] == '\n') {
            dotstuffed.append(buffer[i]);
            linestart = true;
        } else {
            dotstuffed.append(buffer[i]);
            linestart = false;
        }
    }

#ifdef QT_NO_SSL
    waitingForBytes += dotstuffed.length();
#endif
    transport->stream().writeRawData(dotstuffed.constData(), dotstuffed.length());
    //qCDebug(lcSMTP) << "Body: sent a" << bytes << "byte block";
}

void SmtpClient::authExpired()
{
    status = Done;
    operationFailed(QMailServiceAction::Status::ErrConfiguration, tr("Have not received any greeting from SMTP server, probably configuration error"));
    return;
}

void SmtpClient::stopTransferring()
{
    if (temporaryFile)
    {
        if (transport->isEncrypted())
            disconnect(&(transport->socket()), SIGNAL(encryptedBytesWritten(qint64)), this, SLOT(sendMoreData(qint64)));
        else
            disconnect(transport, SIGNAL(bytesWritten(qint64)), this, SLOT(sendMoreData(qint64)));
        delete temporaryFile;
        temporaryFile = 0;
        status = Sent;
    }
}

void SmtpClient::onCredentialsStatusChanged()
{
    qCDebug(lcSMTP)  << "Got credentials status changed:" << credentials->status();
    disconnect(credentials, &QMailCredentialsInterface::statusChanged,
               this, &SmtpClient::onCredentialsStatusChanged);
    if (transport && transport->inUse() && (status == Authenticate))
        nextAction(QString());
}
