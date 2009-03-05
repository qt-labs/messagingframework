/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "imapclient.h"
#include "imapconfiguration.h"
#include "imapstrategy.h"
#ifdef QMAIL_QTOPIA
#include <private/longstream_p.h>
#else
#include <longstream_p.h>
#endif
#include <qmaillog.h>
#include <qmailfolder.h>
#include <qmailnamespace.h>
#include <limits.h>
#include <QFile>
#include <QDir>

namespace {

    QString decodeModifiedBase64(QString in)
    {
        //remove  & -
        in.remove(0,1);
        in.remove(in.length()-1,1);

        if(in.isEmpty())
            return "&";

        QByteArray buf(in.length(),static_cast<char>(0));
        QByteArray out(in.length() * 3 / 4 + 2,static_cast<char>(0));

        //chars to numeric
        QByteArray latinChars = in.toLatin1();
        for (int x = 0; x < in.length(); x++) {
            int c = latinChars[x];
            if ( c >= 'A' && c <= 'Z')
                buf[x] = c - 'A';
            if ( c >= 'a' && c <= 'z')
                buf[x] = c - 'a' + 26;
            if ( c >= '0' && c <= '9')
                buf[x] = c - '0' + 52;
            if ( c == '+')
                buf[x] = 62;
            if ( c == ',')
                buf[x] = 63;
        }

        int i = 0; //in buffer index
        int j = i; //out buffer index

        unsigned char z;
        QString result;

        while(i+1 < buf.size())
        {
            out[j] = buf[i] & (0x3F); //mask out top 2 bits
            out[j] = out[j] << 2;
            z = buf[i+1] >> 4;
            out[j] = (out[j] | z);      //first byte retrieved

            i++;
            j++;

            if(i+1 >= buf.size())
                break;

            out[j] = buf[i] & (0x0F);   //mask out top 4 bits
            out[j] = out[j] << 4;
            z = buf[i+1] >> 2;
            z &= 0x0F;
            out[j] = (out[j] | z);      //second byte retrieved

            i++;
            j++;

            if(i+1 >= buf.size())
                break;

            out[j] = buf[i] & 0x03;   //mask out top 6 bits
            out[j] = out[j] <<  6;
            z = buf[i+1];
            out[j] = out[j] | z;  //third byte retrieved

            i+=2; //next byte
            j++;
        }

        //go through the buffer and extract 16 bit unicode network byte order
        for(int z = 0; z < out.count(); z+=2) {
            unsigned short outcode = 0x0000;
            outcode = out[z];
            outcode <<= 8;
            outcode &= 0xFF00;

            unsigned short b = 0x0000;
            b = out[z+1];
            b &= 0x00FF;
            outcode = outcode | b;
            if(outcode)
                result += QChar(outcode);
        }

        return result;
    }

    QString decodeModUTF7(QString in)
    {
        QRegExp reg("&[^&-]*-");

        int startIndex = 0;
        int endIndex = 0;

        startIndex = in.indexOf(reg,endIndex);
        while (startIndex != -1) {
            endIndex = startIndex;
            while(endIndex < in.length() && in[endIndex] != '-')
                endIndex++;
            endIndex++;

            //extract the base64 string from the input string
            QString mbase64 = in.mid(startIndex,(endIndex - startIndex));
            QString unicodeString = decodeModifiedBase64(mbase64);

            //remove encoding
            in.remove(startIndex,(endIndex-startIndex));
            in.insert(startIndex,unicodeString);

            endIndex = startIndex + unicodeString.length();
            startIndex = in.indexOf(reg,endIndex);
        }

        return in;
    }

    QString decodeFolderName(const QString &name)
    {
        return decodeModUTF7(name);
    }

}

ImapClient::ImapClient(QObject* parent)
    : QObject(parent),
      _retryDelay(5), // seconds
      _waitingForIdle(false)
{
    static int count(0);
    ++count;
    _idleProtocol.setObjectName(QString("I:%1").arg(count));
    _protocol.setObjectName(QString("%1").arg(count));
    _strategyContext = new ImapStrategyContext(this);
    _strategyContext->setStrategy(&_strategyContext->synchronizeAccountStrategy);
    connect(&_protocol, SIGNAL(completed(ImapCommand, OperationStatus)),
            this, SLOT(commandCompleted(ImapCommand, OperationStatus)) );
    connect(&_protocol, SIGNAL(mailboxListed(QString&,QString&,QString&)),
            this, SLOT(mailboxListed(QString&,QString&,QString&)) );
    connect(&_protocol, SIGNAL(messageFetched(QMailMessage&)),
            this, SLOT(messageFetched(QMailMessage&)) );
    connect(&_protocol, SIGNAL(dataFetched(QString, QString, QString, int, bool)),
            this, SLOT(dataFetched(QString, QString, QString, int, bool)) );
    connect(&_protocol, SIGNAL(nonexistentUid(QString)),
            this, SLOT(nonexistentUid(QString)) );
    connect(&_protocol, SIGNAL(messageStored(QString)),
            this, SLOT(messageStored(QString)) );
    connect(&_protocol, SIGNAL(messageCopied(QString, QString)),
            this, SLOT(messageCopied(QString, QString)) );
    connect(&_protocol, SIGNAL(downloadSize(QString, int)),
            this, SLOT(downloadSize(QString, int)) );
    connect(&_protocol, SIGNAL(updateStatus(QString)),
            this, SLOT(transportStatus(QString)) );
    connect(&_protocol, SIGNAL(connectionError(int,QString)),
            this, SLOT(transportError(int,QString)) );
    connect(&_protocol, SIGNAL(connectionError(QMailServiceAction::Status::ErrorCode,QString)),
            this, SLOT(transportError(QMailServiceAction::Status::ErrorCode,QString)) );

    _inactiveTimer.setSingleShot(true);
    connect(&_inactiveTimer, SIGNAL(timeout()),
            this, SLOT(connectionInactive()));

    connect(&_idleProtocol, SIGNAL(continuationRequired(ImapCommand, QString)),
            this, SLOT(idleContinuation(ImapCommand, QString)) );
    connect(&_idleProtocol, SIGNAL(completed(ImapCommand, OperationStatus)),
            this, SLOT(idleCommandTransition(ImapCommand, OperationStatus)) );
    connect(&_idleProtocol, SIGNAL(updateStatus(QString)),
            this, SLOT(transportStatus(QString)));
    connect(&_idleProtocol, SIGNAL(connectionError(int,QString)),
            this, SLOT(idleTransportError()) );
    connect(&_idleProtocol, SIGNAL(connectionError(QMailServiceAction::Status::ErrorCode,QString)),
            this, SLOT(idleTransportError()) );

    _idleTimer.setSingleShot(true);
    connect(&_idleTimer, SIGNAL(timeout()),
            this, SLOT(idleTimeOut()));
}

ImapClient::~ImapClient()
{
    if (_protocol.inUse()) {
        _protocol.close();
    }
    if (_idleProtocol.inUse()) {
        _idleProtocol.close();
    }
}

void ImapClient::newConnection()
{
    if (_protocol.inUse()) {
        _inactiveTimer.stop();
    } else {
        // Reload the account configuration
        _config = QMailAccountConfiguration(_config.id());
    }

    ImapConfiguration imapCfg(_config);
    if ( imapCfg.mailServer().isEmpty() ) {
        operationFailed(QMailServiceAction::Status::ErrConfiguration, tr("Cannot send message without IMAP server configuration"));
        return;
    }

    _strategyContext->newConnection();
}

ImapStrategyContext *ImapClient::strategyContext()
{
    return _strategyContext;
}

ImapStrategy *ImapClient::strategy() const
{
    return _strategyContext->strategy();
}

void ImapClient::setStrategy(ImapStrategy *strategy)
{
    _strategyContext->setStrategy(strategy);
}

void ImapClient::idleContinuation(ImapCommand command, const QString &type)
{
    const int idleTimeout = 28*60*1000;

    if (command == IMAP_Idle) {
        if (type == QString("idling")) {
            qMailLog(IMAP) << "IDLE: Idle connection established.";
            
            // We are now idling
            _idleTimer.start( idleTimeout );

            if (_waitingForIdle)
                commandCompleted(IMAP_Idle_Continuation, OpOk);
        } else {
            qMailLog(IMAP) << "IDLE: event occurred";
            // An event occurred during idle 
            QMailAccount account(_config.id());
            if (account.status() & QMailAccount::Synchronized) {
                // This account is no longer synchronized
                account.setStatus(QMailAccount::Synchronized, false);
                QMailStore::instance()->updateAccount(&account);

                // TODO: it may be better to stop and reissue the idle command here?
                /* 
                _idleTimer.stop();
                _idleProtocol.sendIdleDone();
                */

                emit idleChangeNotification();
            }
        }
    }
}

void ImapClient::idleCommandTransition(const ImapCommand command, const OperationStatus status)
{
    if ( status != OpOk ) {
        idleTransportError();
        commandCompleted(IMAP_Idle_Continuation, OpOk);
        return;
    }
    
    switch( command ) {
        case IMAP_Init:
        {
            _idleProtocol.sendCapability();
            return;
        }
        case IMAP_Capability:
        {
#ifndef QT_NO_OPENSSL
            if (!_idleProtocol.encrypted()) {
                ImapConfiguration imapCfg(_config);
                if (imapCfg.mailEncryption() == QMailTransport::Encrypt_TLS) {
                    if (_idleProtocol.supportsCapability("STARTTLS")) {
                        _idleProtocol.sendStartTLS();
                        break;
                    } else {
                        qWarning() << "No TLS support - continuing unencrypted!";
                    }
                }
            }
#endif

            _idleProtocol.sendLogin(_config);
            return;
        }
        case IMAP_StartTLS:
        {
            _idleProtocol.sendLogin(_config);
            break;
        }
        case IMAP_Login:
        {
            // Find the inbox for this account
            // TODO: the monitored folder should be configurable
            QMailAccount account(_config.id());
            _idleProtocol.sendSelect(QMailFolder(mailboxId("INBOX")));
            return;
        }
        case IMAP_Select:
        {
            _idleProtocol.sendIdle();
            return;
        }
        case IMAP_Idle:
        {
            // Restart idling (TODO: unless we're closing)
            _idleProtocol.sendIdle();
            return;
        }
        default:        //default = all critical messages
        {
            qMailLog(IMAP) << "IDLE: IMAP Idle unknown command response: " << command;
            return;
        }
    }
}

void ImapClient::idleTimeOut()
{
    _idleTimer.stop();
    _idleProtocol.sendIdleDone();
}

void ImapClient::commandCompleted(ImapCommand command, OperationStatus status)
{    
    checkCommandResponse(command, status);
    commandTransition(command, status);
}

void ImapClient::checkCommandResponse(ImapCommand command, OperationStatus status)
{
    if ( status != OpOk ) {
        switch ( command ) {
            case IMAP_UIDStore:
            {
                // Couldn't set a flag, ignore as we can stil continue
                qMailLog(IMAP) << "could not store message flag";
                break;
            }

            case IMAP_Login:
            {
                operationFailed(QMailServiceAction::Status::ErrLoginFailed, _protocol.lastError());
                return;
            }

            case IMAP_Full:
            {
                operationFailed(QMailServiceAction::Status::ErrFileSystemFull, _protocol.lastError());
                return;
            }

            default:        //default = all critical messages
            {
                QString msg;
                if (_config.id().isValid()) {
                    ImapConfiguration imapCfg(_config);
                    msg = imapCfg.mailServer() + ": ";
                }
                msg.append(_protocol.lastError());

                operationFailed(QMailServiceAction::Status::ErrUnknownResponse, msg);
                return;
            }
        }
    }
    
    switch (command) {
        case IMAP_Full:
            // fall through
        case IMAP_Unconnected:
            qFatal( "Logic error, IMAP_Full" );
            break;
        default:
            break;
    }
    
}

void ImapClient::commandTransition(ImapCommand command, OperationStatus status)
{
    switch( command ) {
        case IMAP_Init:
        {
            emit updateStatus( tr("Checking capabilities" ) );
            _protocol.sendCapability();
            break;
        }
        
        case IMAP_Capability:
        {
            ImapConfiguration imapCfg(_config);

#ifndef QT_NO_OPENSSL
            if (!_protocol.encrypted() && (imapCfg.mailEncryption() == QMailTransport::Encrypt_TLS)) {
                if (_protocol.supportsCapability("STARTTLS")) {
                    emit updateStatus( tr("Starting TLS" ) );
                    _protocol.sendStartTLS();
                    break;
                } else {
                    // TODO: request user direction!!!
                    qWarning() << "No TLS support - continuing unencrypted";
                    emit updateStatus( tr("No TLS support - continuing unencrypted") );
                }
            }
#endif

            if (!_idleProtocol.connected()
                && _protocol.supportsCapability("IDLE")
                && imapCfg.pushEnabled()) {
                _waitingForIdle = true;
                emit updateStatus( tr("Logging in idle connection" ) );
                _idleProtocol.open(imapCfg);
            } else {
                if (!imapCfg.pushEnabled() && _idleProtocol.connected())
                    _idleProtocol.close();
                emit updateStatus( tr("Logging in" ) );
                _protocol.sendLogin(_config);
            }
            break;
        }
        
        case IMAP_Idle_Continuation:
        {
            _waitingForIdle = false;
            emit updateStatus( tr("Logging in" ) );
            _protocol.sendLogin(_config);
            break;
        }
        
        case IMAP_StartTLS:
        {
            // Check capabilities for encrypted mode
            _protocol.sendCapability();
            break;
        }

        case IMAP_Select:
        case IMAP_Examine:
        {
            if (_protocol.mailbox().isSelected()) {
                QMailFolder folder(_protocol.mailbox().id);
                QString uidValidity(_protocol.mailbox().uidValidity);
                
                folder.setServerUnreadCount(_protocol.mailbox().unseen);
                folder.setCustomField("qmf-uidvalidity", uidValidity);

                if (folder.serverCount() != static_cast<uint>(_protocol.mailbox().exists)) {
                    folder.setServerCount(_protocol.mailbox().exists);

                    // See how this compares to the local mailstore count
                    updateFolderCountStatus(&folder);
                }

                QMailStore::instance()->updateFolder(&folder);
            }
            // fall through
        }
        
        default:
        {
            _strategyContext->commandTransition(command, status);
            break;
        }
    }
}

/*  Mailboxes retrieved from the server goes here.  If the INBOX mailbox
    is new, it means it is the first time the account is used.
*/
void ImapClient::mailboxListed(QString &flags, QString &delimiter, QString &path)
{
    QMailFolderId parentId;
    QMailFolderId boxId;

    QMailAccount account(_config.id());

    QString mailboxPath;
    QStringList list = path.split(delimiter);
    for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
        if (!mailboxPath.isEmpty())
            mailboxPath.append(delimiter);
        mailboxPath.append(*it);

        boxId = mailboxId(mailboxPath);
        if (boxId.isValid()) {
            // This element already exists
            if (mailboxPath == path) {
                QMailFolder folder(boxId);
                _strategyContext->mailboxListed(folder, flags);
            }

            parentId = boxId;
        } else {
            // This element needs to be created
            QMailFolder folder(mailboxPath, parentId, _config.id());
            folder.setDisplayName(decodeFolderName(*it));
            folder.setStatus(QMailFolder::SynchronizationEnabled, true);

            _strategyContext->mailboxListed(folder, flags);

            boxId = mailboxId(mailboxPath);
        }
    }
}

void ImapClient::messageFetched(QMailMessage& mail)
{
    // Do we need to update the message from the existing data?
    QMailMessageMetaData existing(mail.serverUid(), _config.id());
    if (existing.id().isValid()) {
        bool downloaded(mail.status() & QMailMessage::Downloaded);
        bool replied(mail.status() & QMailMessage::Replied);
        bool readElsewhere(mail.status() & QMailMessage::ReadElsewhere);

        mail.setId(existing.id());
        mail.setStatus(existing.status());
        mail.setContent(existing.content());
        mail.setReceivedDate(existing.receivedDate());
        mail.setPreviousParentFolderId(existing.previousParentFolderId());
        mail.setContentScheme(existing.contentScheme());
        mail.setContentIdentifier(existing.contentIdentifier());
        mail.setCustomFields(existing.customFields());

        // Preserve the status flags determined by the protocol
        mail.setStatus(QMailMessage::Downloaded, downloaded);
        mail.setStatus(QMailMessage::Replied, replied);
        mail.setStatus(QMailMessage::ReadElsewhere, readElsewhere);
    } else {
        mail.setStatus(QMailMessage::Incoming, true);
        mail.setStatus(QMailMessage::New, true);
    }

    mail.setMessageType(QMailMessage::Email);
    mail.setParentAccountId(_config.id());
    mail.setParentFolderId(_protocol.mailbox().id);

    _classifier.classifyMessage(mail);

    _strategyContext->messageFetched(mail);
}

static bool updateParts(QMailMessagePart &part, const QByteArray &bodyData)
{
    static const QByteArray newLine(QMailMessage::CRLF);
    static const QByteArray marker("--");
    static const QByteArray bodyDelimiter(newLine + newLine);

    if (part.multipartType() == QMailMessage::MultipartNone) {
        // The body data is for this part only
        part.setBody(QMailMessageBody::fromData(bodyData, part.contentType(), part.transferEncoding(), QMailMessageBody::AlreadyEncoded));
    } else {
        const QByteArray &boundary(part.contentType().boundary());

        // TODO: this code is replicated from qmailmessage.cpp (parseMimeMultipart)

        // Separate the body into parts delimited by the boundary, and update them individually
        QByteArray partDelimiter = marker + boundary;
        QByteArray partTerminator = newLine + partDelimiter + marker;

        int startPos = bodyData.indexOf(partDelimiter, 0);
        if (startPos != -1)
            startPos += partDelimiter.length();

        // Subsequent delimiters include the leading newline
        partDelimiter.prepend(newLine);

        const char *baseAddress = bodyData.constData();
        int partIndex = 0;

        int endPos = bodyData.indexOf(partTerminator, 0);
        while ((startPos != -1) && (startPos < endPos)) {
            // Skip the boundary line
            startPos = bodyData.indexOf(newLine, startPos);

            if ((startPos != -1) && (startPos < endPos)) {
                // Parse the section up to the next boundary marker
                int nextPos = bodyData.indexOf(partDelimiter, startPos);

                // Find the beginning of the part body
                startPos = bodyData.indexOf(bodyDelimiter, startPos);
                if ((startPos != -1) && (startPos < nextPos)) {
                    startPos += bodyDelimiter.length();

                    QByteArray partBodyData(QByteArray::fromRawData(baseAddress + startPos, (nextPos - startPos)));
                    if (!updateParts(part.partAt(partIndex), partBodyData))
                        return false;

                    ++partIndex;
                }

                // Move to the next part
                startPos = nextPos + partDelimiter.length();
            }
        }
    }

    return true;
}

static bool updatePartFile(const QString &partFileStr, const QString &chunkFileStr)
{
    QFile partFile(partFileStr);
    QFile chunkFile(chunkFileStr);
    if (!partFile.exists()) {
        if (!QFile::copy(chunkFileStr, partFileStr)) {
            return false;
        }
    } else if (partFile.open(QIODevice::Append) 
               && chunkFile.open(QIODevice::ReadOnly)) {
            char buffer[4096];
            qint64 readSize;
            while (!chunkFile.atEnd()) {
                readSize = chunkFile.read(buffer, 4096);
                if (readSize == -1)
                    return false;
                if (partFile.write(buffer, readSize) != readSize)
                    return false;
            }
    } else {
        return false;
    }
    return true;
}

void ImapClient::dataFetched(const QString &uid, const QString &section, const QString &chunkName, int size, bool partial)
{
    QMailMessagePart::Location partLocation;
    if (!section.isEmpty())
        partLocation = QMailMessagePart::Location(section);
    QString fileName = chunkName;

    if (!uid.isEmpty() && (section.isEmpty() || partLocation.isValid(false))) {
        QMailMessage mail(uid, _config.id());
        if (mail.id().isValid()) {
            QString tempDir = QMail::tempPath() + QDir::separator();
            QString partFileStr(tempDir + "mail-" + uid + "-part-" + section);
            if (partial && size) {
                if (!updatePartFile(partFileStr, chunkName)
                    || !QFile::remove(chunkName)
                    || !QFile::copy(partFileStr, chunkName)) {
                    operationFailed(QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to store fetched data"));
                    return;
                }
                fileName = chunkName;
            } else if (partial && !size) { // Complete part retrieved
                if (!QFile::remove(chunkName)
                    || !QFile::rename(partFileStr, chunkName)) {
                    operationFailed(QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to store fetched data"));
                    return;
                }
            }
                
            // Update the relevant part
            QMailMessagePart &part = mail.partAt(partLocation);
            if (section.isEmpty()) {
                mail.setBody(QMailMessageBody::fromFile(fileName, mail.contentType(), mail.transferEncoding(), QMailMessageBody::AlreadyEncoded));
                
            } else if (part.multipartType() == QMailMessage::MultipartNone) {
                // The body data is for this part only
                if (part.hasBody()) {
                    qWarning() << "Updating existing part body - uid:" << uid << "section:" << section;
                }
                part.setBody(QMailMessageBody::fromFile(fileName, part.contentType(), part.transferEncoding(), QMailMessageBody::AlreadyEncoded));

                // Only use one detached file at a time
                if (mail.customField("qtopiamail-detached-filename").isEmpty()) {
                    // The file can be used directly if the transfer was not encoded
                    if ((part.transferEncoding() != QMailMessageBody::Base64) && (part.transferEncoding() != QMailMessageBody::QuotedPrintable)) {
                        // The file we wrote to is detached, and the mailstore can assume ownership
                        mail.setCustomField("qtopiamail-detached-filename", fileName);
                    }
                } 
            } else {
                // Find the part bodies in the retrieved data
                QFile file(fileName);
                if (!file.open(QIODevice::ReadOnly)) {
                    qWarning() << "Unable to read fetched data from:" << fileName << "- error:" << file.error();
                    operationFailed(QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to read fetched data"));
                    return;
                }

                uchar *data = file.map(0, size);
                if (!data) {
                    qWarning() << "Unable to map fetched data from:" << fileName << "- error:" << file.error();
                    operationFailed(QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to map fetched data"));
                    return;
                }

                // Update the part bodies that we have retrieved
                QByteArray bodyData(QByteArray::fromRawData(reinterpret_cast<const char*>(data), size));
                if (!updateParts(part, bodyData)) {
                    operationFailed(QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to update part body"));
                    return;
                }

                // These updates are not supported by the file data
                if (!mail.customField("qtopiamail-detached-filename").isEmpty()) {
                    mail.removeCustomField("qtopiamail-detached-filename");
                }
            }

            _strategyContext->dataFetched(mail, uid, section);
        }
    } else {
        qWarning() << "Unable to handle dataFetched - uid:" << uid << "section:" << section;
        operationFailed(QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to handle dataFetched without context"));
    }
}

void ImapClient::nonexistentUid(const QString &uid)
{
    _strategyContext->nonexistentUid(uid);
}

void ImapClient::messageStored(const QString &uid)
{
    _strategyContext->messageStored(uid);
}

void ImapClient::messageCopied(const QString &copiedUid, const QString &createdUid)
{
    _strategyContext->messageCopied(copiedUid, createdUid);
}

void ImapClient::downloadSize(const QString &uid, int size)
{
    _strategyContext->downloadSize(uid, size);
}

void ImapClient::setAccount(const QMailAccountId &id)
{
    if (_protocol.inUse() && (id != _config.id())) {
        operationFailed(QMailServiceAction::Status::ErrConnectionInUse, tr("Cannot send message; socket in use"));
        return;
    }

    _config = QMailAccountConfiguration(id);
}

QMailAccountId ImapClient::account() const
{
    return _config.id();
}

void ImapClient::transportError(int code, const QString &msg)
{
    operationFailed(code, msg);
}

void ImapClient::transportError(QMailServiceAction::Status::ErrorCode code, const QString &msg)
{
    operationFailed(code, msg);
}

void ImapClient::idleTransportError()
{
    qMailLog(IMAP) << "IDLE: An IMAP IDLE related error occurred.\n"
                   << "An attempt to automatically recover is scheduled in" << _retryDelay << "seconds.";

    if (_idleProtocol.inUse())
        _idleProtocol.close();

    updateStatus(tr("Idle Error occurred"));
    QTimer::singleShot(_retryDelay*1000, this, SLOT(idleErrorRecovery()));
}

void ImapClient::idleErrorRecovery()
{
    const int oneHour = 60*60;
    if (_idleProtocol.connected()) {
        qMailLog(IMAP) << "IDLE: IMAP IDLE error recovery was successful.";
        _retryDelay = 5;
        return;
    }
    qMailLog(IMAP) << "IDLE: IMAP IDLE error recovery failed. Retrying in" << _retryDelay << "seconds.";
    _retryDelay = qMin( oneHour, _retryDelay*2 );
    QTimer::singleShot(_retryDelay*1000, this, SLOT(idleErrorRecovery()));

    if (_protocol.inUse())
        return;
    _protocol.close();
    newConnection();
}

void ImapClient::closeConnection()
{
    _inactiveTimer.stop();

    if ( _protocol.connected() ) {
        emit updateStatus( tr("Logging out") );
        _protocol.sendLogout();
    } else if ( _protocol.inUse() ) {
        _protocol.close();
    }
}

void ImapClient::transportStatus(const QString& status)
{
    emit updateStatus(status);
}

void ImapClient::cancelTransfer()
{
    operationFailed(QMailServiceAction::Status::ErrCancel, tr("Cancelled by user"));
}

void ImapClient::retrieveOperationCompleted()
{
    // This retrieval may have been asynchronous
    emit allMessagesReceived();

    // Or it may have been requested by a waiting client
    emit retrievalCompleted();

    deactivateConnection();
}

void ImapClient::deactivateConnection()
{
    const int inactivityPeriod = 20 * 1000;

    _inactiveTimer.start(inactivityPeriod);
}

void ImapClient::connectionInactive()
{
    closeConnection();
}

void ImapClient::operationFailed(int code, const QString &text)
{
    if (_protocol.inUse())
        _protocol.close();

    emit errorOccurred(code, text);
}

void ImapClient::operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    if (_protocol.inUse())
        _protocol.close();

    emit errorOccurred(code, text);
}

QMailFolderId ImapClient::mailboxId(const QString &path) const
{
    QMailFolderIdList folderIds = QMailStore::instance()->queryFolders(QMailFolderKey::parentAccountId(_config.id()) & QMailFolderKey::path(path));
    if (folderIds.count() == 1)
        return folderIds.first();
    
    return QMailFolderId();
}

QMailFolderIdList ImapClient::mailboxIds() const
{
    return QMailStore::instance()->queryFolders(QMailFolderKey::parentAccountId(_config.id()), QMailFolderSortKey::path(Qt::AscendingOrder));
}

QStringList ImapClient::serverUids(const QMailFolderId &folderId) const
{
    // We need to list both the messages in the mailbox, and those moved to the
    // Trash folder which are still in the mailbox as far as the server is concerned
    return serverUids(messagesKey(folderId) | trashKey(folderId));
}

QStringList ImapClient::serverUids(const QMailFolderId &folderId, quint64 messageStatusFilter, bool set) const
{
    QMailMessageKey statusKey(QMailMessageKey::status(messageStatusFilter, QMailDataComparator::Includes));
    return serverUids((messagesKey(folderId) | trashKey(folderId)) & (set ? statusKey : ~statusKey));
}

QStringList ImapClient::serverUids(QMailMessageKey key) const
{
    QStringList uidList;

    foreach (const QMailMessageMetaData& r, QMailStore::instance()->messagesMetaData(key, QMailMessageKey::ServerUid))
        uidList.append(r.serverUid());

    return uidList;
}

QMailMessageKey ImapClient::messagesKey(const QMailFolderId &folderId) const
{
    return (QMailMessageKey::parentAccountId(_config.id()) &
            QMailMessageKey::parentFolderId(folderId));
}

QMailMessageKey ImapClient::trashKey(const QMailFolderId &folderId) const
{
    return (QMailMessageKey::parentAccountId(_config.id()) &
            QMailMessageKey::parentFolderId(QMailFolderId(QMailFolder::TrashFolder)) &
            QMailMessageKey::previousParentFolderId(folderId));
}

QStringList ImapClient::deletedMessages(const QMailFolderId &folderId) const
{
    QStringList serverUidList;

    foreach (const QMailMessageRemovalRecord& r, QMailStore::instance()->messageRemovalRecords(_config.id(), folderId))
        serverUidList.append(r.serverUid());

    return serverUidList;
}

void ImapClient::updateFolderCountStatus(QMailFolder *folder)
{
    // Find the local mailstore count for this folder
    QMailMessageKey folderContent(QMailMessageKey::parentFolderId(folder->id()));
    QMailMessageKey previousFolderContent(QMailMessageKey::previousParentFolderId(folder->id()));
    QMailMessageKey trashContent(QMailMessageKey::parentFolderId(QMailFolderId(QMailFolder::TrashFolder)));

    uint count = QMailStore::instance()->countMessages(folderContent | (previousFolderContent & trashContent));

    folder->setStatus(QMailFolder::PartialContent, (count < folder->serverCount()));
}

