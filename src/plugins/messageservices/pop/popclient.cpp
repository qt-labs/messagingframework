/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "popclient.h"
#include "popauthenticator.h"
#include "popconfiguration.h"
#include <QFileInfo>
#include <longstream_p.h>
#include <longstring_p.h>
#include <qmailstore.h>
#include <qmailtransport.h>
#include <qmaillog.h>
#include <qmaildisconnected.h>

#include <limits.h>

PopClient::PopClient(QObject* parent)
    : QObject(parent),
      selected(false),
      deleting(false),
      headerLimit(0),
      additional(0),
      partialContent(false),
      dataStream(new LongStream),
      transport(0)
{
    inactiveTimer.setSingleShot(true);
    connect(&inactiveTimer, SIGNAL(timeout()), this, SLOT(connectionInactive()));
}

PopClient::~PopClient()
{
    delete dataStream;
    delete transport;
}

QMailMessage::MessageType PopClient::messageType() const
{
    return QMailMessage::Email;
}

void PopClient::newConnection()
{
    if (transport && transport->connected()) {
        if (selected) {
            // Re-use the existing connection
            inactiveTimer.stop();
        } else {
            // We won't get an updated listing until we re-connect
            sendCommand("QUIT");
            status = Exit;
            closeConnection();
        }
    } else {
        // Re-load the configuration for this account
        config = QMailAccountConfiguration(config.id());
    }

    PopConfiguration popCfg(config);
    if ( popCfg.mailServer().isEmpty() ) {
        status = Exit;
        operationFailed(QMailServiceAction::Status::ErrConfiguration, tr("Cannot open connection without POP server configuration"));
        return;
    }

    if (!selected) {
        serverUidNumber.clear();
        serverUid.clear();
        serverSize.clear();
        obsoleteUids.clear();
        newUids.clear();
        messageCount = 0;
    }

    if (transport && transport->connected() && selected) {
        if (deleting) {
            uidlIntegrityCheck();
        }

        // Retrieve the specified messages
        status = RequestMessage;
        nextAction();
    } else {
        if (!transport) {
            // Set up the transport
            transport = new QMailTransport("POP");

            connect(transport, SIGNAL(updateStatus(QString)), this, SIGNAL(updateStatus(QString)));
            connect(transport, SIGNAL(connected(QMailTransport::EncryptType)), this, SLOT(connected(QMailTransport::EncryptType)));
            connect(transport, SIGNAL(errorOccurred(int,QString)), this, SLOT(transportError(int,QString)));
            connect(transport, SIGNAL(readyRead()), this, SLOT(incomingData()));
        }

        status = Init;
        capabilities.clear();
        transport->open(popCfg.mailServer(), popCfg.mailPort(), static_cast<QMailTransport::EncryptType>(popCfg.mailEncryption()));
    }
}

void PopClient::setAccount(const QMailAccountId &id)
{
    if ((transport && transport->inUse()) && (id != config.id())) {
        QString msg("Cannot open account; transport in use");
        emit errorOccurred(QMailServiceAction::Status::ErrConnectionInUse, msg);
        return;
    }

    config = QMailAccountConfiguration(id);
}

QMailAccountId PopClient::account() const
{
    return config.id();
}

void PopClient::setOperation(QMailRetrievalAction::RetrievalSpecification spec)
{
    selected = false;
    deleting = false;
    additional = 0;
    QMailAccount account(config.id());

    if (spec == QMailRetrievalAction::MetaData) {
        PopConfiguration popCfg(config);

        if (popCfg.isAutoDownload()) {
            // Just download everything
            headerLimit = INT_MAX;
        } else {
            headerLimit = popCfg.maxMailSize() * 1024;
        }
    } else {
        headerLimit = 0;
    }
    
    // get/create child folder
    QMailFolderIdList folderList = QMailStore::instance()->queryFolders(QMailFolderKey::parentAccountId(account.id()));
    if (folderList.count() > 1) {
        qWarning() << "Pop account has more than one child folder, account" << account.id();
        folderId = folderList.first();
    } else if (folderList.count() == 1) {
        folderId = folderList.first();
    } else {
        QMailFolder childFolder("Inbox", QMailFolderId(), account.id());
        childFolder.setDisplayName(tr("Inbox"));
        childFolder.setStatus(QMailFolder::SynchronizationEnabled, true);
        childFolder.setStatus(QMailFolder::Incoming, true);

        if(!QMailStore::instance()->addFolder(&childFolder))
            qWarning() << "Unable to add child folder to pop account";
        folderId = childFolder.id();
    }
    partialContent = QMailFolder(folderId).status() & QMailFolder::PartialContent;
}

void PopClient::setAdditional(uint _additional)
{
    additional = _additional;
}

void PopClient::setDeleteOperation()
{
    deleting = true;
}

void PopClient::setSelectedMails(const SelectionMap& data)
{
    // We shouldn't have anything left in our retrieval list...
    if (!retrievalSize.isEmpty()) {
        foreach (const QString& uid, retrievalSize.keys())
            qMailLog(IMAP) << "Message" << uid << "still in retrieve map...";

        retrievalSize.clear();
    }

    selected = true;
    selectionMap = data;
    selectionItr = selectionMap.begin();
    completionList.clear();
    messageCount = 0;

    if (deleting == false) {
        totalRetrievalSize = 0;
        foreach (const QMailMessageId& id, selectionMap.values()) {
            QMailMessageMetaData message(id);
            uint size = message.indicativeSize();
            uint bytes = message.size();

            retrievalSize.insert(message.serverUid(), qMakePair(qMakePair(size, bytes), 0u));
            totalRetrievalSize += size;
        }

        // Report the total size we will retrieve
        progressRetrievalSize = 0;
        emit progressChanged(progressRetrievalSize, totalRetrievalSize);
    }
}

void PopClient::connected(QMailTransport::EncryptType encryptType)
{
    PopConfiguration popCfg(config);
    if (popCfg.mailEncryption() == encryptType) {
        qMailLog(POP) << "Connected" << flush;
        emit updateStatus(tr("Connected"));
    }

#ifndef QT_NO_OPENSSL
    if ((popCfg.mailEncryption() != QMailTransport::Encrypt_SSL) && (status == TLS)) {
        // We have entered TLS mode - restart the connection
        capabilities.clear();
        status = Init;
        nextAction();
    }
#endif
}

void PopClient::transportError(int status, QString msg)
{
    operationFailed(status, msg);
}

void PopClient::closeConnection()
{
    inactiveTimer.stop();

    if (transport) {
        if (transport->connected()) {
            if ( status == Exit ) {
                // We have already sent our quit command
                transport->close();
            } else {
                // Send a quit command
                status = Quit;
                nextAction();
            }
        } else if (transport->inUse()) {
            transport->close();
        }
    }
}

void PopClient::sendCommand(const char *data, int len)
{
    if (len == -1)
        len = ::strlen(data);

    QDataStream &out(transport->stream());
    out.writeRawData(data, len);
    out.writeRawData("\r\n", 2);

    if (len){
        QString logData(data);
        QRegExp passExp("^PASS\\s");
        if (passExp.indexIn(logData) != -1) {
            logData = logData.left(passExp.matchedLength()) + "<password hidden>";
        }
        
        qMailLog(POP) << "SEND:" << logData;
    }
}

void PopClient::sendCommand(const QString& cmd)
{
    sendCommand(cmd.toAscii());
}

void PopClient::sendCommand(const QByteArray& cmd)
{
    sendCommand(cmd.data(), cmd.length());
}

QString PopClient::readResponse() 
{
    QString response = transport->readLine();

    if (response.length() > 1){
        qMailLog(POP) << "RECV:" << qPrintable(response.left(response.length() - 2));
    }

    return response;
}

void PopClient::incomingData()
{
    while (transport->canReadLine()) {
        QString response = readResponse();
        processResponse(response);
    }
}

void PopClient::processResponse(const QString &response)
{
    bool waitForInput = false;

    switch (status) {
    case Init:
    {
        if (!response.isEmpty()) {
            // See if there is an APOP timestamp in the greeting
            QRegExp timeStampPattern("<\\S+>");
            if (timeStampPattern.indexIn(response) != -1)
                capabilities.append(QString("APOP:") + timeStampPattern.cap(0));
        }
        break;
    }
    case CapabilityTest:
    {
        if (response[0] != '+') {
            // CAPA command not supported - just continue without it
            status = Capabilities;
        }
        break;
    }
    case Capabilities:
    {
        QString capability(response.left(response.length() - 2));
        if (!capability.isEmpty() && (capability != QString("."))) {
            capabilities.append(capability);

            // More to follow
            waitForInput = true;
        }
        break;
    }
    case TLS:
    {
        if (response[0] != '+') {
            // Unable to initiate TLS 
            operationFailed(QMailServiceAction::Status::ErrLoginFailed, "");
        } else {
            // Switch into encrypted mode and wait for encrypted connection event
#ifndef QT_NO_OPENSSL
            transport->switchToEncrypted();
            waitForInput = true;
#endif
        }
        break;
    }
    case Auth:
    {
        if (response[0] != '+') {
            // Authentication failed
            operationFailed(QMailServiceAction::Status::ErrLoginFailed, "");
        } else {
            if ((response.length() > 2) && (response[1] == ' ')) {
                // This is a continuation containing a challenge string (in Base64)
                QByteArray challenge = QByteArray::fromBase64(response.mid(2).toAscii());
                QByteArray response(PopAuthenticator::getResponse(config.serviceConfiguration("pop3"), challenge));

                if (!response.isEmpty()) {
                    // Send the response as Base64 encoded
                    sendCommand(response.toBase64());
                    waitForInput = true;
                }
            } else {
                if (!authCommands.isEmpty()) {
                    // Multiple-command authentication protcol
                    sendCommand(authCommands.takeFirst());
                    waitForInput = true;
                } else {
                    // We have finished authenticating!
                }
            }
        }
        break;
    }
    case Uidl:
    {
        if (response[0] != '+') {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }
    case UidList:
    {
        QString input(response.left(response.length() - 2));
        if (!input.isEmpty() && (input != QString("."))) {
            // Extract the number and UID
            QRegExp pattern("(\\d+) +(.*)");
            if (pattern.indexIn(input) != -1) {
                int number(pattern.cap(1).toInt());
                QString uid(pattern.cap(2));

                serverUidNumber.insert(uid, number);
                serverUid.insert(number, uid);
            }

            // More to follow
            waitForInput = true;
        }
        break;
    }
    case List:
    {
        if (response[0] != '+') {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }
    case SizeList:
    {
        QString input(response.left(response.length() - 2));
        if (!input.isEmpty() && (input != QString("."))) {
            // Extract the number and size
            QRegExp pattern("(\\d+) +(\\d+)");
            if (pattern.indexIn(input) != -1) {
                serverSize.insert(pattern.cap(1).toInt(), pattern.cap(2).toUInt());
            }

            waitForInput = true;
        }
        break;
    }
    case Retr:
    {
        if (response[0] != '+') {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }
    case MessageData:
    {
        if (response != QString(".\r\n")) {
            if (response.startsWith(".")) {
                // This line has been byte-stuffed
                dataStream->append(response.mid(1));
            } else {
                dataStream->append(response);
            }

            if (dataStream->status() == LongStream::OutOfSpace) {
                operationFailed(QMailServiceAction::Status::ErrFileSystemFull, LongStream::errorMessage( "\n" ));
            } else {
                // More message data remains
                waitForInput = true;

                if (!retrieveUid.isEmpty() && !transport->canReadLine()) {
                    // There is no more data currently available, so report our progress
                    RetrievalMap::iterator it = retrievalSize.find(retrieveUid);
                    if (it != retrievalSize.end()) {
                        QPair<QPair<uint, uint>, uint> &values = it.value();

                        // Calculate the percentage of the retrieval completed
                        uint totalBytes = values.first.second;
                        uint percentage = totalBytes ? qMin<uint>(dataStream->length() * 100 / totalBytes, 100) : 100;

                        if (percentage > values.second) {
                            values.second = percentage;

                            // Update the progress figure to count the retrieved portion of this message
                            uint partialSize = values.first.first * percentage / 100;
                            emit progressChanged(progressRetrievalSize + partialSize, totalRetrievalSize);
                        }
                    }
                }
            }
        } else {
            // Received the terminating line - we now have the full message data
            createMail();
        }
        break;
    }
    case Dele:
    {
        if (response[0] != '+') {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }
    case Exit:
    {
        transport->close();        //close regardless of response
        break;
    }

    // The following cases do not relate to input processing
    case StartTLS:
    case Connected:
    case RequestUids:
    case RequestSizes:
    case RequestMessage:
    case DeleteMessage:
    case Done:
    case Quit:
        qWarning() << "processResponse requested from inappropriate state:" << status;
        break;
    }

    // Are we waiting for further input?
    if (!waitForInput && transport->inUse()) {
        // Go on to the next action
        nextAction();
    }
}

void PopClient::nextAction()
{
    int nextStatus = -1;
    QString nextCommand;
    bool waitForInput = false;

    switch (status) {
    case Init:
    {
        // See if the server will tell us its capabilities
        nextStatus = CapabilityTest;
        nextCommand = "CAPA";
        break;
    }
    case CapabilityTest:
    {
        // Capability data will follow
        nextStatus = Capabilities;
        waitForInput = true;
        break;
    }
    case Capabilities:
    {
        // Try going on to TLS negotiation
        nextStatus = StartTLS;
        break;
    }
    case StartTLS:
    {
        if (!transport->isEncrypted()) {
            if (PopAuthenticator::useEncryption(config.serviceConfiguration("pop3"), capabilities)) {
                // Switch to TLS mode
                nextStatus = TLS;
                nextCommand = "STLS";
                break;
            }
        }

        // We're connected - try to authenticate
        nextStatus = Connected;
        break;
    }
    case Connected:
    {
        emit updateStatus(tr("Logging in"));

        // Get the login command sequence to use
        authCommands = PopAuthenticator::getAuthentication(config.serviceConfiguration("pop3"), capabilities);

        nextStatus = Auth;
        nextCommand = authCommands.takeFirst();
        break;
    }
    case Auth:
    {
        // we're authenticated - get the list of UIDs
        nextStatus = RequestUids;
        break;
    }
    case RequestUids:
    {
        nextStatus = Uidl;
        nextCommand = "UIDL";
        break;
    }
    case Uidl:
    {
        // UID list data will follow
        nextStatus = UidList;
        waitForInput = true;
        break;
    }
    case UidList:
    {
        // We have the list of available messages
        uidlIntegrityCheck();

        if (!newUids.isEmpty()) {
            // Proceed to get the message sizes for the reported messages
            nextStatus = RequestSizes;
        } else {
            nextStatus = RequestMessage;
        }
        break;
    }
    case RequestSizes:
    {
        nextStatus = List;
        nextCommand = "LIST";
        break;
    }
    case List:
    {
        // Size list data will follow
        nextStatus = SizeList;
        waitForInput = true;
        break;
    }
    case SizeList:
    {
        // We now have the list of messages and their sizes
        nextStatus = RequestMessage;
        break;
    }
    case RequestMessage:
    {
        int msgNum = nextMsgServerPos();
        if (msgNum != -1) {
            if (!selected) {
                if (messageCount == 1)
                    emit updateStatus(tr("Previewing","Previewing <no of messages>") +QChar(' ') + QString::number(newUids.count()));
            } else {
                emit updateStatus(tr("Completing %1 / %2").arg(messageCount).arg(selectionMap.count()));
            }

            nextStatus = Retr;
            QString temp = QString::number(msgNum);
            if (selected || ((headerLimit > 0) && (mailSize <= headerLimit))) {
                // Retrieve the whole message
                nextCommand = ("RETR " + temp);
            } else {                                //only header
                nextCommand = ("TOP " + temp + " 0");
            }
        } else {
            // No more messages to be fetched - are there any to be deleted?
            if (!obsoleteUids.isEmpty()) {
                qMailLog(POP) << qPrintable(QString::number(obsoleteUids.count()) + " messages in mailbox to be deleted");
                emit updateStatus(tr("Removing old messages"));

                nextStatus = DeleteMessage;
            } else {
                nextStatus = Done;
            }
        }
        break;
    }
    case Retr:
    {
        // Message data will follow
        message = "";
        dataStream->reset();

        nextStatus = MessageData;
        waitForInput = true;
        break;
    }
    case MessageData:
    {
        // See if there are more messages to retrieve
        nextStatus = RequestMessage;
        break;
    }
    case DeleteMessage:
    {
        int pos = -1;
        while ((pos == -1) && !obsoleteUids.isEmpty()) {
            QString uid = obsoleteUids.takeFirst();
            QMailStore::instance()->purgeMessageRemovalRecords(config.id(), QStringList() << uid);

            pos = msgPosFromUidl(uid);
            if (pos == -1) {
                qMailLog(POP) << "Not sending delete for unlisted UID:" << uid;
                if (deleting) {
                    QMailMessageKey accountKey(QMailMessageKey::parentAccountId(config.id()));
                    QMailMessageKey uidKey(QMailMessageKey::serverUid(uid));

                    QMailStore::instance()->removeMessages(accountKey & uidKey, QMailStore::NoRemovalRecord);
                }
            } else {
                messageUid = uid;
            }
        }

        if (pos != -1) {
            nextStatus = Dele;
            nextCommand = ("DELE " + QString::number(pos));
        } else {
            nextStatus = Done;
        }
        break;
    }
    case Dele:
    {
        if (deleting) {
            QMailMessageKey accountKey(QMailMessageKey::parentAccountId(config.id()));
            QMailMessageKey uidKey(QMailMessageKey::serverUid(messageUid));

            messageProcessed(messageUid);
            QMailStore::instance()->removeMessages(accountKey & uidKey, QMailStore::NoRemovalRecord);
        }

        // See if there are more messages to delete
        nextStatus = DeleteMessage;
        break;
    }
    case Done:
    {
        if (!selected && !completionList.isEmpty()) {
            // Fetch any messages that need completion
            setSelectedMails(completionList);

            nextStatus = RequestMessage;
        } else {
            // We're all done
            retrieveOperationCompleted();
            waitForInput = true;
        }
        break;
    }
    case Quit:
    {
        emit updateStatus(tr("Logging out"));
        nextStatus = Exit;
        nextCommand = "QUIT";
        break;
    }
    case Exit:
    {
        transport->close();        //close regardless
        waitForInput = true;
        break;
    }

    // The following cases do not initiate actions:
    case TLS:
        qWarning() << "nextAction requested from inappropriate state:" << status;
        waitForInput = true;
        break;
    }

    // Are we changing state here?
    if (nextStatus != -1) {
        status = static_cast<TransferStatus>(nextStatus);
    }

    if (!nextCommand.isEmpty()) {
        sendCommand(nextCommand);
    } else if (!waitForInput) {
        // Go on to the next action
        nextAction();
    }
}

int PopClient::msgPosFromUidl(QString uidl)
{
    QMap<QString, int>::const_iterator it = serverUidNumber.find(uidl);
    if (it != serverUidNumber.end())
        return it.value();

    return -1;
}

int PopClient::nextMsgServerPos()
{
    int thisMsg = -1;

    if (!selected) {
        // Processing message listing
        if ( messageCount < newUids.count() ) {
            messageUid = newUids.at(messageCount);
            thisMsg = msgPosFromUidl(messageUid);
            mailSize = getSize(thisMsg);
            messageCount++;
        }
    } else {
        // Retrieving a specified list
        QString serverId;
        if (selectionItr != selectionMap.end()) {
            serverId = selectionItr.key();
            selectionItr++;
            ++messageCount;
        }

        // if requested mail is not on server, try to get a new mail from the list
        while ( (thisMsg == -1) && !serverId.isEmpty() ) {
            int pos = msgPosFromUidl(serverId);
            if (pos == -1) {
                // Mark this message as deleted
                QMailMessage message(selectionMap[serverId]);
                if (message.id().isValid()) {
                    message.setStatus(QMailMessage::Removed, true);
                    QMailStore::instance()->updateMessage(&message);
                }

                messageProcessed(serverId);

                if (selectionItr != selectionMap.end()) {
                    serverId = selectionItr.key();
                    selectionItr++;
                } else {
                    serverId = QString();
                }
            } else {
                thisMsg = pos;
                messageUid = serverId;
                mailSize = getSize(thisMsg);
            }
        }

        if (!serverId.isEmpty())
            retrieveUid = serverId;
    }

    return thisMsg;
}

// get the reported server size from stored list
uint PopClient::getSize(int pos)
{
    QMap<int, uint>::const_iterator it = serverSize.find(pos);
    if (it != serverSize.end())
        return it.value();

    return -1;
}

void PopClient::uidlIntegrityCheck()
{
    if (deleting) {
        newUids.clear();

        // Only delete the messages that were specified
        obsoleteUids = selectionMap.keys();
        selectionItr = selectionMap.end();
    } else if (!selected) {
        // Find the existing UIDs for this account
        QStringList messageUids;
        QMailMessageKey key(QMailMessageKey::parentAccountId(config.id()));
        foreach (const QMailMessageMetaData& r, QMailStore::instance()->messagesMetaData(key, QMailMessageKey::ServerUid))
            messageUids.append(r.serverUid());

        // Find the locally-deleted UIDs for this account
        QStringList deletedUids;
        foreach (const QMailMessageRemovalRecord& r, QMailStore::instance()->messageRemovalRecords(config.id()))
            deletedUids.append(r.serverUid());

        obsoleteUids = QStringList();

        PopConfiguration popCfg(config);

        // Create list of new entries that should be downloaded
        bool gapFilled(false); // need to 'fill gap' (retrieve messages newly arrived on server since last sync)
        uint gap(0);
        if (messageUids.isEmpty()) {
            // no 'gap' to fill
            gapFilled = true;
        }
        QMapIterator<int, QString> it(serverUid);
        QString uid;
        it.toBack();
        while (it.hasPrevious()) {
            it.previous();
            uid = it.value();
            obsoleteUids.removeAll(uid);

            if (deletedUids.contains(uid)) {
                // This message is deleted locally and present on the server
                deletedUids.removeAll(uid);

                if (popCfg.canDeleteMail())
                    obsoleteUids.append(uid);
            } else {
                if (messageUids.contains(uid)) {
                    gapFilled = true;
                } else {
                    // This message is not present locally, new to client.
                    newUids.append(uid);
                    // measure gap
                    if (!gapFilled)
                        ++gap;
                }
            }
        }

        messageCount = 0;

        if (!deletedUids.isEmpty()) {
            // Remove any deletion records for messages not available from the server any more
            QMailStore::instance()->purgeMessageRemovalRecords(config.id(), deletedUids);
            foreach (const QString &uid, deletedUids)
                messageProcessed(uid);
        }

        // Partial pop retrieving is not working on gmail
        // Does not seem possible to support it in a cross platform way.
        const bool partialPopRetrievalWorking = false;
        partialContent = false;
        
        // Update partialContent status for the account
        if (additional && partialPopRetrievalWorking) {
            additional += gap;
            partialContent = uint(newUids.count()) > additional;
            // When minimum is set, only retrieve minimum ids
            newUids = newUids.mid(0, additional);
        }
    }
}

namespace {

struct AttachmentDetector 
{
    bool operator()(const QMailMessagePart &part)
    {
        // Return false if there is an attachment to stop traversal
        QMailMessageContentDisposition disposition(part.contentDisposition());
        return (disposition.isNull() || (disposition.type() != QMailMessageContentDisposition::Attachment));
    }
};

bool hasAttachments(const QMailMessagePartContainer &partContainer)
{
    // If foreachPart yields false there is at least one attachment
    return (partContainer.foreachPart(AttachmentDetector()) == false);
}

}

void PopClient::createMail()
{
    int detachedSize = dataStream->length();
    QString detachedFile = dataStream->detach();
    
    QMailMessage mail = QMailMessage::fromRfc2822File( detachedFile );

    mail.setSize(mailSize);
    mail.setServerUid(messageUid);
    mail.setCustomField( "qtopiamail-detached-filename", detachedFile );

    if (selectionMap.contains(mail.serverUid())) {
        // We need to update the message from the existing data
        QMailMessageMetaData existing(selectionMap.value(mail.serverUid()));

        mail.setId(existing.id());
        mail.setStatus(existing.status());
        mail.setContent(existing.content());
        QMailDisconnected::copyPreviousFolder(existing, &mail);
        mail.setContentScheme(existing.contentScheme());
        mail.setContentIdentifier(existing.contentIdentifier());
        mail.setCustomFields(existing.customFields());
    } else {
        mail.setStatus(QMailMessage::Incoming, true);
        mail.setStatus(QMailMessage::New, true);
        mail.setReceivedDate(QMailTimeStamp::currentDateTime());
    }

    mail.setMessageType(QMailMessage::Email);
    mail.setParentAccountId(config.id());

    mail.setParentFolderId(folderId);

    bool isComplete = (selected || ((headerLimit > 0) && (mailSize <= headerLimit)));
    mail.setStatus(QMailMessage::ContentAvailable, isComplete);
    mail.setStatus(QMailMessage::PartialContentAvailable, isComplete);
    if (!isComplete) {
        mail.setContentSize(mailSize - detachedSize);
    }

    if (isComplete && (mail.multipartType() != QMailMessage::MultipartNone)) {
        // See if any of the parts are attachments
        if (hasAttachments(mail)) {
            mail.setStatus( QMailMessage::HasAttachments, true );
        }
    }

    classifier.classifyMessage(mail);

    // Store this message to the mail store
    if (mail.id().isValid()) {
        QMailStore::instance()->updateMessage(&mail);
    } else {
        int matching = QMailStore::instance()->countMessages(QMailMessageKey::serverUid(mail.serverUid()) & QMailMessageKey::parentAccountId(mail.parentAccountId()));
        if (matching == 0) {
            QMailStore::instance()->addMessage(&mail);
        } else {
            if (matching > 1)
                qWarning() << "Updating only 1 of many duplicate messages (" << mail.serverUid() << "). Account: " << mail.parentAccountId();

            mail.setId(QMailStore::instance()->message(mail.serverUid(), mail.parentAccountId()).id());
            QMailStore::instance()->updateMessage(&mail);
        }
    }

    if (isComplete && !mail.serverUid().isEmpty()) {
        // We have now retrieved the entire message
        messageProcessed(mail.serverUid());

        if (retrieveUid == mail.serverUid()) {
            retrieveUid = QString();
        }
    }

    dataStream->reset();

    // Catch all cleanup of detached file
    QFile::remove(detachedFile);
    
    // Workaround for message buffer file being deleted 
    QFileInfo newFile(dataStream->fileName());
    if (!newFile.exists()) {
        qWarning() << "Unable to find message buffer file, pop protocol";
        dataStream->detach();
    }
    
}

void PopClient::checkForNewMessages()
{
    // We can't have new messages without contacting the server
    emit allMessagesReceived();
}

void PopClient::cancelTransfer(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    operationFailed(code, text);
}

void PopClient::retrieveOperationCompleted()
{
    if (!deleting && !selected) {
        // Only update PartialContent flag when retrieving message list
        QMailFolder folder(folderId);
        folder.setStatus(QMailFolder::PartialContent, partialContent);
        if(!QMailStore::instance()->updateFolder(&folder))
            qWarning() << "Unable to update folder" << folder.id() << "to set PartialContent";
    }
        
    // This retrieval may have been asynchronous
    emit allMessagesReceived();

    // Or it may have been requested by a waiting client
    emit retrievalCompleted();

    deactivateConnection();
}

void PopClient::deactivateConnection()
{
    const int inactivityPeriod = 20 * 1000;

    inactiveTimer.start(inactivityPeriod);
    selected = false;
}

void PopClient::connectionInactive()
{
    closeConnection();
}

void PopClient::messageProcessed(const QString &uid)
{
    RetrievalMap::iterator it = retrievalSize.find(uid);
    if (it != retrievalSize.end()) {
        // Update the progress figure
        progressRetrievalSize += it.value().first.first;
        emit progressChanged(progressRetrievalSize, totalRetrievalSize);

        retrievalSize.erase(it);
    }

    emit messageActionCompleted(uid);
}

void PopClient::operationFailed(int code, const QString &text)
{
    if (transport && transport->inUse()) {
        transport->close();
    }

    emit errorOccurred(code, text);
}

void PopClient::operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    if (transport && transport->inUse()) {
        transport->close();
    }

    QString msg;
    if (code == QMailServiceAction::Status::ErrUnknownResponse) {
        if (config.id().isValid()) {
            PopConfiguration popCfg(config);
            msg = popCfg.mailServer() + ": ";
        }
    }
    msg.append(text);

    emit errorOccurred(code, msg);
}

