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

#include "popclient.h"
#include "popauthenticator.h"
#include "popconfiguration.h"
#include <QFileInfo>
#include <private/longstream_p.h>
#include <qmailstore.h>
#include <qmailmessagebuffer.h>
#include <qmailtransport.h>
#include <qmaillog.h>
#include <qmaildisconnected.h>
#include <limits.h>
#if !defined(Q_OS_WIN)
#include <unistd.h>
#endif


class MessageFlushedWrapper : public QMailMessageBufferFlushCallback
{
    PopClient *context;
    bool isComplete;
public:
    MessageFlushedWrapper(PopClient *_context, bool _isComplete)
        : context(_context)
        , isComplete(_isComplete)
    {
    }

    void messageFlushed(QMailMessage *message)
    {
        context->messageFlushed(*message, isComplete);
        context->removeAllFromBuffer(message);
    }
};

PopClient::PopClient(QObject* parent)
    : QObject(parent),
      selected(false),
      deleting(false),
      headerLimit(0),
      additional(0),
      partialContent(false),
      dataStream(new LongStream),
      transport(0),
      testing(false),
      pendingDeletes(false)
{
    inactiveTimer.setSingleShot(true);
    connect(&inactiveTimer, SIGNAL(timeout()), this, SLOT(connectionInactive()));
    connect(QMailMessageBuffer::instance(), SIGNAL(flushed()), this, SLOT(messageBufferFlushed()));
}

PopClient::~PopClient()
{
    foreach (QMailMessageBufferFlushCallback * c, callbacks) {
        QMailMessageBuffer::instance()->removeCallback(c);
    }

    delete dataStream;
    delete transport;
}

void PopClient::messageBufferFlushed()
{
    callbacks.clear();
}

QMailMessage::MessageType PopClient::messageType() const
{
    return QMailMessage::Email;
}

void PopClient::createTransport()
{
    if (!transport) {
        // Set up the transport
        transport = new QMailTransport("POP");

        connect(transport, SIGNAL(updateStatus(QString)), this, SIGNAL(updateStatus(QString)));
        connect(transport, SIGNAL(connected(QMailTransport::EncryptType)), this, SLOT(connected(QMailTransport::EncryptType)));
        connect(transport, SIGNAL(errorOccurred(int,QString)), this, SLOT(transportError(int,QString)));
        connect(transport, SIGNAL(readyRead()), this, SLOT(incomingData()));
    }
}

void PopClient::deleteTransport()
{
    if (transport) {
        // Need to immediately disconnect these signals or slots may try to use null transport object
        disconnect(transport, SIGNAL(updateStatus(QString)), this, SIGNAL(updateStatus(QString)));
        disconnect(transport, SIGNAL(connected(QMailTransport::EncryptType)), this, SLOT(connected(QMailTransport::EncryptType)));
        disconnect(transport, SIGNAL(errorOccurred(int,QString)), this, SLOT(transportError(int,QString)));
        disconnect(transport, SIGNAL(readyRead()), this, SLOT(incomingData()));

        // A Qt socket remains in an unusuable state for a short time after closing,
        // thus it can't be immediately reused
        transport->deleteLater();
        transport = 0;
    }
}

void PopClient::testConnection()
{
    testing = true;
    pendingDeletes = false;
    closeConnection();

    PopConfiguration popCfg(config);
    if ( popCfg.mailServer().isEmpty() ) {
        status = Exit;
        operationFailed(QMailServiceAction::Status::ErrConfiguration, tr("Cannot open connection without POP server configuration"));
        return;
    }
    
    createTransport();

    status = Init;
    capabilities.clear();
    transport->open(popCfg.mailServer(), popCfg.mailPort(), static_cast<QMailTransport::EncryptType>(popCfg.mailEncryption()));
}

void PopClient::newConnection()
{
    testing = false;
    pendingDeletes = false;
    lastStatusTimer.start();
    if (transport && transport->connected()) {
        if (selected) {
            // Re-use the existing connection
            inactiveTimer.stop();
        } else {
            // We won't get an updated listing until we re-connect
            closeConnection();
        }
    }
    // Re-load the configuration for this account
    config = QMailAccountConfiguration(config.id());

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
        createTransport();

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
    QMailAccount account(id);
    if (account.status() & QMailAccount::CanCreateFolders) {
        account.setStatus(QMailAccount::CanCreateFolders, false);
        if (!QMailStore::instance()->updateAccount(&account)) {
            qWarning() << "Unable to update account" << account.id() << "to CanCreateFolders" << false;
        } else {
            qMailLog(POP) << "CanCreateFolders for " << account.id() << "changed to" << false;
        }
    }
    // Update non-local folders which have 'RenamePermitted=true'/'DeletionPermitted=true'/'ChildCreationPermitted=true'/'MessagesPermitted=false'
    QMailFolderKey popKey = QMailFolderKey::parentAccountId(id);
    popKey &= QMailFolderKey::id(QMailFolder::LocalStorageFolderId, QMailDataComparator::NotEqual);
    popKey &= QMailFolderKey::ancestorFolderIds(QMailFolderId(QMailFolder::LocalStorageFolderId), QMailDataComparator::Excludes);
    popKey &= QMailFolderKey::status(QMailFolder::DeletionPermitted, QMailDataComparator::Includes)
            | QMailFolderKey::status(QMailFolder::RenamePermitted, QMailDataComparator::Includes)
            | QMailFolderKey::status(QMailFolder::ChildCreationPermitted, QMailDataComparator::Includes)
            | QMailFolderKey::status(QMailFolder::MessagesPermitted, QMailDataComparator::Excludes);

    QMailFolderIdList folderIds = QMailStore::instance()->queryFolders(popKey);
    foreach (const QMailFolderId &folderId, folderIds) {
        QMailFolder folder = QMailFolder(folderId);
        folder.setStatus(QMailFolder::DeletionPermitted, false);
        folder.setStatus(QMailFolder::RenamePermitted, false);
        folder.setStatus(QMailFolder::ChildCreationPermitted, false);
        folder.setStatus(QMailFolder::MessagesPermitted, true);
        if (!QMailStore::instance()->updateFolder(&folder)) {
            qWarning() << "Unable to update flags for POP folder" << folder.id() << folder.path();
        } else {
            qMailLog(POP) <<  "Flags for POP folder" << folder.id() << folder.path() << "updated";
        }
    }
}

QMailAccountId PopClient::accountId() const
{
    return config.id();
}

bool PopClient::synchronizationEnabled(const QMailFolderId &id) const
{
    return id.isValid() // not accountChecking
        || (QMailFolder(folderId).status() & QMailFolder::SynchronizationEnabled);
}

void PopClient::setOperation(QMailRetrievalAction::RetrievalSpecification spec)
{
    selected = false;
    deleting = false;
    additional = 0;

    switch (spec) {
    case QMailRetrievalAction::Auto:
        {
            // Re-load the configuration for this account
            QMailAccountConfiguration accountCfg(config.id());
            PopConfiguration popCfg(accountCfg);

            if (popCfg.isAutoDownload()) {
                // Just download everything
                headerLimit = UINT_MAX;
            } else {
                headerLimit = popCfg.maxMailSize() * 1024;
            }
        }
        break;
    case QMailRetrievalAction::Content:
        headerLimit = UINT_MAX;
        break;
    case QMailRetrievalAction::MetaData:
    case QMailRetrievalAction::Flags:
    default:
        headerLimit = 0;
        break;
    }

    findInbox();
}

// Returns true if inbox already exists, otherwise returns false
bool PopClient::findInbox()
{
    bool result = false;
    QMailAccount account(config.id());

    // get/create child folder
    QMailFolderIdList folderList = QMailStore::instance()->queryFolders(QMailFolderKey::parentAccountId(account.id()));
    if (folderList.count() > 1) {
        qWarning() << "Pop account has more than one child folder, account" << account.id();
        folderId = folderList.first();
        result = true;
    } else if (folderList.count() == 1) {
        folderId = folderList.first();
        result = true;
    } else {
        QMailFolder childFolder("Inbox", QMailFolderId(), account.id());
        childFolder.setDisplayName(tr("Inbox"));
        childFolder.setStatus(QMailFolder::SynchronizationEnabled, true);
        childFolder.setStatus(QMailFolder::Incoming, true);
        childFolder.setStatus(QMailFolder::MessagesPermitted, true);

        if(!QMailStore::instance()->addFolder(&childFolder))
            qWarning() << "Unable to add child folder to pop account";
        folderId = childFolder.id();
        account.setStandardFolder(QMailFolder::InboxFolder, folderId);
        if (!QMailStore::instance()->updateAccount(&account)) {
            qWarning() << "Unable to update account" << account.id();
        }
    }
    partialContent = QMailFolder(folderId).status() & QMailFolder::PartialContent;
    return result;
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
            qMailLog(POP) << "Message" << uid << "still in retrieve map...";

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

#ifndef QT_NO_SSL
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
                sendCommand("QUIT");
                status = Exit;
                transport->close();
            }
        } else if (transport->inUse()) {
            transport->close();
        }
    }
    deleteTransport();
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
    sendCommand(cmd.toLatin1());
}

void PopClient::sendCommand(const QByteArray& cmd)
{
    sendCommand(cmd.data(), cmd.length());
}

void PopClient::incomingData()
{
    if (!lineBuffer.isEmpty() && (transport && transport->canReadLine())) {
        processResponse(QString::fromLatin1(lineBuffer + transport->readLine()));
        lineBuffer.clear();
    }

    while (transport && transport->canReadLine()) {
        processResponse(QString::fromLatin1(transport->readLine()));
    }

    if (transport && transport->bytesAvailable()) {
        // If there is an incomplete line, read it from the socket buffer to ensure we get readyRead signal next time
        lineBuffer.append(transport->readAll());
    }
}

void PopClient::processResponse(const QString &response)
{
    if ((response.length() > 1) && (status != MessageDataRetr) && (status != MessageDataTop)) {
        qMailLog(POP) << "RECV:" << qPrintable(response.left(response.length() - 2));
    }

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
        if (!capability.isEmpty() && (capability != QString('.'))) {
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
#ifndef QT_NO_SSL
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
                QByteArray challenge = QByteArray::fromBase64(response.mid(2).toLatin1());
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
        if (!input.isEmpty() && (input != QString('.'))) {
            // Extract the number and UID
            QRegExp pattern("(\\d+) +(.*)");
            if (pattern.indexIn(input) != -1) {
                int number(pattern.cap(1).toInt());
                QString uid(pattern.cap(2));

                serverUidNumber.insert(uid.toLatin1(), number);
                serverUid.insert(number, uid.toLatin1());
            }

            // More to follow
            waitForInput = true;
        }
        if (lastStatusTimer.elapsed() > 1000) {
            lastStatusTimer.start();
            emit progressChanged(0, 0);
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
        if (!input.isEmpty() && (input != QString('.'))) {
            // Extract the number and size
            QRegExp pattern("(\\d+) +(\\d+)");
            if (pattern.indexIn(input) != -1) {
                serverSize.insert(pattern.cap(1).toInt(), pattern.cap(2).toUInt());
            }

            waitForInput = true;
        }
        if (lastStatusTimer.elapsed() > 1000) {
            lastStatusTimer.start();
            emit progressChanged(0, 0);
        }
        break;
    }
    case Retr:
    case Top:
    {
        if (response[0] != '+') {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }
    case MessageDataRetr:
    case MessageDataTop:
    {
        if (response != QString(".\r\n")) {
            if (response.startsWith('.')) {
                // This line has been byte-stuffed
                dataStream->append(response.mid(1));
            } else {
                dataStream->append(response);
            }

            if (dataStream->status() == LongStream::OutOfSpace) {
                operationFailed(QMailServiceAction::Status::ErrFileSystemFull,
                                LongStream::errorMessage(QString('\n')));
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
    case DeleAfterRetr:
    case Dele:
    {
        if (response[0] != '+') {
            operationFailed(QMailServiceAction::Status::ErrUnknownResponse, response);
        }
        break;
    }
    case Exit:
    {
        closeConnection(); //close regardless of response
        retrieveOperationCompleted();
        waitForInput = true;
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
    if (!waitForInput && transport && transport->inUse()) {
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
        if (testing) {
            nextStatus = Done;
        } else {
            // we're authenticated - get the list of UIDs
            nextStatus = RequestUids;
        }
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
                if (messageCount == 1) {
                    emit updateStatus(tr("Previewing","Previewing <no of messages>") +QChar(' ') + QString::number(newUids.count()));
                }
                if (lastStatusTimer.elapsed() > 1000) {
                    lastStatusTimer.start();
                    emit progressChanged(messageCount, newUids.count());
                }
            } else {
                emit updateStatus(tr("Completing %1 / %2").arg(messageCount).arg(selectionMap.count()));
            }

            QString temp = QString::number(msgNum);
            if ((headerLimit > 0) && (mailSize <= headerLimit)) {
                // Retrieve the whole message
                nextCommand = ("RETR " + temp);
                nextStatus = Retr;
            } else {                                //only header
                nextCommand = ("TOP " + temp + " 0");
                nextStatus = Top;
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
    case Top:
    {
        // Message data will follow
        message = "";
        dataStream->reset();

        nextStatus = (status == Retr) ? MessageDataRetr : MessageDataTop;
        waitForInput = true;
        break;
    }
    case MessageDataRetr:
    {
        PopConfiguration popCfg(config);
        if (popCfg.deleteRetrievedMailsFromServer()) {
            // Now that sqlite WAL is used, make sure mail metadata is sync'd 
            // on device before removing mail from external mail server
            QMailStore::instance()->ensureDurability();
            int pos = msgPosFromUidl(messageUid);
            emit updateStatus(tr("Removing message from server"));
            nextCommand = ("DELE " + QString::number(pos));
            nextStatus = DeleAfterRetr;
        } else {
            // See if there are more messages to retrieve
            nextStatus = RequestMessage;
        }
        break;
    }
    case MessageDataTop:
    {
        // See if there are more messages to retrieve
        nextStatus = RequestMessage;
        break;
    }
    case DeleAfterRetr:
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
                pendingDeletes = true;
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
        } else if (pendingDeletes) {
            nextStatus = Quit;
        } else {
            // We're all done
            retrieveOperationCompleted();
            waitForInput = true;
            nextStatus = Quit;
        }
        break;
    }
    case Quit:
    {
        emit updateStatus(tr("Logging out"));
        nextStatus = Exit;
        nextCommand = "QUIT";
        waitForInput = true;
        break;
    }
    case Exit:
    {
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
    QMap<QByteArray, int>::const_iterator it = serverUidNumber.find(uidl.toLocal8Bit());
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
            QMailMessage message(selectionMap[serverId]);
            if (pos == -1) {
                // Mark this message as deleted
                if (message.id().isValid()) {
                    message.setStatus(QMailMessage::Removed, true);
                    QMailStore::instance()->updateMessage(&message);
                }

                messageProcessed(serverId);

                if (selectionItr != selectionMap.end()) {
                    serverId = selectionItr.key();
                    selectionItr++;
                } else {
                    serverId.clear();
                }
            } else {
                thisMsg = pos;
                messageUid = serverId;
                mailSize = getSize(thisMsg);
                if (mailSize == uint(-1) && message.id().isValid()) {
                    mailSize = message.size();
                }
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
        QMapIterator<int, QByteArray> it(serverUid);
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

void PopClient::createMail()
{
    int detachedSize = dataStream->length();
    QString detachedFile = dataStream->detach();

    qMailLog(POP) << qPrintable(QString("RECV: <%1 message bytes received>").arg(detachedSize));
    
    QMailMessage *mail(new QMailMessage(QMailMessage::fromRfc2822File(detachedFile)));
    _bufferedMessages.append(mail);

    mail->setSize(mailSize);
    mail->setServerUid(messageUid);

    if (selectionMap.contains(mail->serverUid())) {
        // We need to update the message from the existing data
        QMailMessageMetaData existing(selectionMap.value(mail->serverUid()));

        mail->setId(existing.id());
        mail->setStatus(existing.status());
        mail->setContent(existing.content());
        QMailDisconnected::copyPreviousFolder(existing, mail);
        mail->setContentScheme(existing.contentScheme());
        mail->setContentIdentifier(existing.contentIdentifier());
        mail->setCustomFields(existing.customFields());
    } else {
        mail->setStatus(QMailMessage::Incoming, true);
        mail->setStatus(QMailMessage::New, true);
        mail->setReceivedDate(mail->date());
    }
    mail->setCustomField( "qmf-detached-filename", detachedFile );

    mail->setMessageType(QMailMessage::Email);
    mail->setParentAccountId(config.id());

    mail->setParentFolderId(folderId);

    bool isComplete = ((headerLimit > 0) && (mailSize <= headerLimit));
    mail->setStatus(QMailMessage::ContentAvailable, isComplete);
    mail->setStatus(QMailMessage::PartialContentAvailable, isComplete);
    if (!isComplete) {
        mail->setContentSize(mailSize - detachedSize);
    }
    if (isComplete) {
        // Mark as LocalOnly if it will be removed from the server after
        // retrieval. The original mail will be deleted from the server
        // by the state machine.
        PopConfiguration popCfg(config);
        if (popCfg.deleteRetrievedMailsFromServer()) {
            mail->setStatus(QMailMessage::LocalOnly, true);
        }
        mail->setStatus(QMailMessage::CalendarInvitation, mail->hasCalendarInvitation());
        mail->setStatus(QMailMessage::HasAttachments, mail->hasAttachments());
    }

    // Special case to handle spurious hotmail messages. Hide in UI, but do not delete from server
    if (mail->from().toString().isEmpty()) {
        mail->setStatus(QMailMessage::Removed, true);
        QFile file(detachedFile);
        QByteArray contents;
        if (file.open(QFile::ReadOnly)) {
            contents = file.read(2048);
        }
        qMailLog(POP) << "Bad message retrieved serverUid" << mail->serverUid() << "contents" << contents;
    }

    classifier.classifyMessage(*mail);

    // Store this message to the mail store
    if (mail->id().isValid()) {
        QMailMessageBuffer::instance()->updateMessage(mail);
    } else {
        QMailMessageKey duplicateKey(QMailMessageKey::serverUid(mail->serverUid()) & QMailMessageKey::parentAccountId(mail->parentAccountId()));
        QMailStore::instance()->removeMessages(duplicateKey);
        QMailMessageBuffer::instance()->addMessage(mail);
    }

    dataStream->reset();


    QMailMessageBufferFlushCallback *callback = new MessageFlushedWrapper(this, isComplete);
    QMailMessageBuffer::instance()->setCallback(mail, callback);
    callbacks.push_back(callback);
}

void PopClient::messageFlushed(QMailMessage &message, bool isComplete)
{
    if (isComplete && !message.serverUid().isEmpty()) {
        // We have now retrieved the entire message
        messageProcessed(message.serverUid());

        if (retrieveUid == message.serverUid()) {
            retrieveUid.clear();
        }
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
    // Flush any batched writes now
    QMailMessageBuffer::instance()->flush();

    if (!deleting && !selected) {
        // Only update PartialContent flag when retrieving message list
        QMailFolder folder(folderId);
        folder.setStatus(QMailFolder::PartialContent, partialContent);
        if(!QMailStore::instance()->updateFolder(&folder))
            qWarning() << "Unable to update folder" << folder.id() << "to set PartialContent";
    }
    
    if (!selected) {
        QMailAccount account(accountId());
        account.setLastSynchronized(QMailTimeStamp::currentDateTime());
        if (!QMailStore::instance()->updateAccount(&account))
            qWarning() << "Unable to update account" << account.id() << "to set lastSynchronized";
    }
        
    // This retrieval may have been asynchronous
    emit allMessagesReceived();

    // Or it may have been requested by a waiting client
    emit retrievalCompleted();

    if (transport) {
        deactivateConnection();
    }
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
        deleteTransport();
    }

    emit errorOccurred(code, text);
}

void PopClient::operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    if (transport && transport->inUse()) {
        transport->close();
        deleteTransport();
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

void PopClient::removeAllFromBuffer(QMailMessage *message)
{
    int i = 0;
    while ((i = _bufferedMessages.indexOf(message, i)) != -1) {
        delete _bufferedMessages.at(i);
        _bufferedMessages.remove(i);
    }
}
