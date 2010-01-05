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

#include "imapclient.h"
#include "imapauthenticator.h"
#include "imapconfiguration.h"
#include "imapstrategy.h"
#include <longstream_p.h>
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

class IdleProtocol : public ImapProtocol {
    Q_OBJECT

public:
    IdleProtocol(ImapClient *client, const QMailFolder &folder);
    virtual ~IdleProtocol() {}

    virtual void handleIdling() {}
    virtual void handleInitialIdling();
    virtual bool open(const ImapConfiguration& config);
    int idleRetryDelay() { return _idleRetryDelay; }
    
signals:
    void idleNewMailNotification(QMailFolderId);
    void idleFlagsChangedNotification(QMailFolderId);
    void openRequest(IdleProtocol*);
    
protected slots:
    virtual void idleContinuation(ImapCommand, const QString &);
    virtual void idleCommandTransition(ImapCommand, OperationStatus);
    virtual void idleTimeOut();
    virtual void idleTransportError();
    virtual void idleErrorRecovery();

protected:
    ImapClient *_client;
    QMailFolder _folder;
    
private:
    QTimer _idleTimer; // Send a DONE command every 29 minutes
    QTimer _idleRecoveryTimer; // Check command hasn't hung
    int _idleRetryDelay; // Try to restablish IDLE state
    enum IdleRetryDelay { InitialIdleRetryDelay = 30 }; //seconds
    bool _initialIdling;
};

IdleProtocol::IdleProtocol(ImapClient *client, const QMailFolder &folder)
    :_idleRetryDelay(InitialIdleRetryDelay),
     _initialIdling(true)
{ 
    _client = client;
    _folder = folder;
    connect(this, SIGNAL(continuationRequired(ImapCommand, QString)),
            this, SLOT(idleContinuation(ImapCommand, QString)) );
    connect(this, SIGNAL(completed(ImapCommand, OperationStatus)),
            this, SLOT(idleCommandTransition(ImapCommand, OperationStatus)) );
    connect(this, SIGNAL(connectionError(int,QString)),
            this, SLOT(idleTransportError()) );
    connect(this, SIGNAL(connectionError(QMailServiceAction::Status::ErrorCode,QString)),
            this, SLOT(idleTransportError()) );

    _idleTimer.setSingleShot(true);
    connect(&_idleTimer, SIGNAL(timeout()),
            this, SLOT(idleTimeOut()));
    _idleRecoveryTimer.setSingleShot(true);
    connect(&_idleRecoveryTimer, SIGNAL(timeout()),
            this, SLOT(idleErrorRecovery()));
}

bool IdleProtocol::open(const ImapConfiguration& config)
{
    _idleRecoveryTimer.start(_idleRetryDelay*1000);
    return ImapProtocol::open(config);
}

void IdleProtocol::handleInitialIdling()
{
    emit idleNewMailNotification(_folder.id());
}

void IdleProtocol::idleContinuation(ImapCommand command, const QString &type)
{
    const int idleTimeout = 28*60*1000;

    if (command == IMAP_Idle) {
        if (type == QString("idling")) {
            qMailLog(IMAP) << "IDLE: Idle connection established.";
            
            // We are now idling
            _idleTimer.start(idleTimeout);
            _idleRecoveryTimer.stop();

            handleIdling();
            if (_initialIdling) {
                _initialIdling = false;
                handleInitialIdling();
            }
        } else if (type == QString("newmail")) {
            qMailLog(IMAP) << "IDLE: new mail event occurred";
            // A new mail event occurred during idle 
            emit idleNewMailNotification(_folder.id());
        } else if (type == QString("flagschanged")) {
            qMailLog(IMAP) << "IDLE: flags changed event occurred";
            // A flags changed event occurred during idle 
            emit idleFlagsChangedNotification(_folder.id());
        } else {
            qWarning("idleContinuation: unknown continuation event");
        }
    }
}

void IdleProtocol::idleCommandTransition(const ImapCommand command, const OperationStatus status)
{
    if ( status != OpOk ) {
        idleTransportError();
        handleIdling();
        return;
    }
    
    QMailAccountConfiguration config(_client->account());
    switch( command ) {
        case IMAP_Init:
        {
            // We need to request the capabilities
            sendCapability();
            return;
        }
        case IMAP_Capability:
        {
            if (!encrypted()) {
                if (ImapAuthenticator::useEncryption(config.serviceConfiguration("imap4"), capabilities())) {
                    // Switch to encrypted mode
                    sendStartTLS();
                    break;
                }
            }

            // We are now connected
            sendLogin(config);
            return;
        }
        case IMAP_StartTLS:
        {
            sendLogin(config);
            break;
        }
        case IMAP_Login:
        {
            sendSelect(_folder);
            return;
        }
        case IMAP_Select:
        {
            sendIdle();
            return;
        }
        case IMAP_Idle:
        {
            // Restart idling (TODO: unless we're closing)
            sendIdle();
            return;
        }
        case IMAP_Logout:
        {
            // Ensure connection is closed on logout
            close();
            return;
        }
        default:        //default = all critical messages
        {
            qMailLog(IMAP) << "IDLE: IMAP Idle unknown command response: " << command;
            return;
        }
    }
}

void IdleProtocol::idleTimeOut()
{
    _idleRecoveryTimer.start(_idleRetryDelay*1000); // Detect an unresponsive server
    _idleTimer.stop();
    sendIdleDone();
}

void IdleProtocol::idleTransportError()
{
    qMailLog(IMAP) << "IDLE: An IMAP IDLE related error occurred.\n"
                   << "An attempt to automatically recover is scheduled in" << _idleRetryDelay << "seconds.";

    if (inUse())
        close();
    
    _idleRecoveryTimer.stop();
    QTimer::singleShot(_idleRetryDelay*1000, this, SLOT(idleErrorRecovery()));
}

void IdleProtocol::idleErrorRecovery()
{
    const int oneHour = 60*60;
    _idleRecoveryTimer.stop();
    if (connected() && _idleTimer.isActive()) {
        qMailLog(IMAP) << "IDLE: IMAP IDLE error recovery was successful. About to check for new mail.";
        _idleRetryDelay = InitialIdleRetryDelay;
        emit idleNewMailNotification(_folder.id()); // Check for new messages arriving while idle connection was down
        emit idleFlagsChangedNotification(_folder.id());
        return;
    }
    updateStatus(tr("Idle Error occurred"));
    QTimer::singleShot(_idleRetryDelay*1000, this, SLOT(idleErrorRecovery()));
    _idleRetryDelay = qMin( oneHour, _idleRetryDelay*2 );
    
    emit openRequest(this);
}

class PrimaryIdleProtocol : public IdleProtocol {
    Q_OBJECT

public:
    PrimaryIdleProtocol(ImapClient *client, const QMailFolder &folder) : IdleProtocol(client, folder) {};
    virtual ~PrimaryIdleProtocol() {};

    virtual void handleIdling() { _client->idling(); }
    virtual void handleInitialIdling() {}

private slots:
    virtual void idleContinuation(ImapCommand, const QString &);
};

void PrimaryIdleProtocol::idleContinuation(ImapCommand command, const QString &type)
{
    if (!(_folder.id().isValid())
        &&_client->mailboxId("INBOX").isValid()) {
        _folder = QMailFolder(_client->mailboxId("INBOX"));
    }
    IdleProtocol::idleContinuation(command, type)       ;
}

ImapClient::ImapClient(QObject* parent)
    : QObject(parent),
      _waitingForIdle(false)
{
    static int count(0);
    ++count;
    QMailFolder idleFolder("INBOX"); // Folders may not have been listed on server
    if (mailboxId("INBOX").isValid()) {
        idleFolder = QMailFolder(mailboxId("INBOX"));
    }
    _idleProtocol = new PrimaryIdleProtocol(this, idleFolder);
    _idleProtocol->setObjectName(QString("I:%1").arg(count));
    _protocol.setObjectName(QString("%1").arg(count));
    _strategyContext = new ImapStrategyContext(this);
    _strategyContext->setStrategy(&_strategyContext->synchronizeAccountStrategy);
    connect(&_protocol, SIGNAL(completed(ImapCommand, OperationStatus)),
            this, SLOT(commandCompleted(ImapCommand, OperationStatus)) );
    connect(&_protocol, SIGNAL(mailboxListed(QString,QString)),
            this, SLOT(mailboxListed(QString,QString)));
    connect(&_protocol, SIGNAL(messageFetched(QMailMessage&)),
            this, SLOT(messageFetched(QMailMessage&)) );
    connect(&_protocol, SIGNAL(dataFetched(QString, QString, QString, int)),
            this, SLOT(dataFetched(QString, QString, QString, int)) );
    connect(&_protocol, SIGNAL(nonexistentUid(QString)),
            this, SLOT(nonexistentUid(QString)) );
    connect(&_protocol, SIGNAL(messageStored(QString)),
            this, SLOT(messageStored(QString)) );
    connect(&_protocol, SIGNAL(messageCopied(QString, QString)),
            this, SLOT(messageCopied(QString, QString)) );
    connect(&_protocol, SIGNAL(messageCreated(QMailMessageId, QString)),
            this, SLOT(messageCreated(QMailMessageId, QString)) );
    connect(&_protocol, SIGNAL(downloadSize(QString, int)),
            this, SLOT(downloadSize(QString, int)) );
    connect(&_protocol, SIGNAL(urlAuthorized(QString)),
            this, SLOT(urlAuthorized(QString)) );
    connect(&_protocol, SIGNAL(folderCreated(QString)),
            this, SLOT(folderCreated(QString)));
    connect(&_protocol, SIGNAL(folderDeleted(QMailFolder)),
            this, SLOT(folderDeleted(QMailFolder)));
    connect(&_protocol, SIGNAL(folderRenamed(QMailFolder, QString)),
            this, SLOT(folderRenamed(QMailFolder, QString)));
    connect(&_protocol, SIGNAL(updateStatus(QString)),
            this, SLOT(transportStatus(QString)) );
    connect(&_protocol, SIGNAL(connectionError(int,QString)),
            this, SLOT(transportError(int,QString)) );
    connect(&_protocol, SIGNAL(connectionError(QMailServiceAction::Status::ErrorCode,QString)),
            this, SLOT(transportError(QMailServiceAction::Status::ErrorCode,QString)) );

    _inactiveTimer.setSingleShot(true);
    connect(&_inactiveTimer, SIGNAL(timeout()),
            this, SLOT(connectionInactive()));

    connect(_idleProtocol, SIGNAL(idleNewMailNotification(QMailFolderId)),
            this, SIGNAL(idleNewMailNotification(QMailFolderId)));
    connect(_idleProtocol, SIGNAL(idleFlagsChangedNotification(QMailFolderId)),
            this, SIGNAL(idleFlagsChangedNotification(QMailFolderId)));
    connect(_idleProtocol, SIGNAL(updateStatus(QString)),
            this, SLOT(transportStatus(QString)));
    connect(_idleProtocol, SIGNAL(openRequest(IdleProtocol *)),
            this, SLOT(idleOpenRequested(IdleProtocol *)));
}

ImapClient::~ImapClient()
{
    if (_protocol.inUse()) {
        _protocol.close();
    }
    if (_idleProtocol->inUse()) {
        _idleProtocol->close();
        delete _idleProtocol;
    }
    foreach(QMailFolderId id, _monitored.keys()) {
        IdleProtocol *protocol = _monitored.take(id);
        if (protocol->inUse())
            protocol->close();
        delete protocol;
    }
}

void ImapClient::newConnection()
{
    if (_protocol.loggingOut())
        _protocol.close();
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

void ImapClient::commandCompleted(ImapCommand command, OperationStatus status)
{
    checkCommandResponse(command, status);
    if (status == OpOk)
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
            qFatal( "Logic error, IMAP_Full" );
            break;
        case IMAP_Unconnected:
            qFatal( "Logic error, Unconnected" );
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
            // We need to request the capabilities
            emit updateStatus( tr("Checking capabilities" ) );
            _protocol.sendCapability();
            break;
        }
        
        case IMAP_Capability:
        {
            if (!_protocol.encrypted()) {
                if (ImapAuthenticator::useEncryption(_config.serviceConfiguration("imap4"), _protocol.capabilities())) {
                    // Switch to encrypted mode
                    emit updateStatus( tr("Starting TLS" ) );
                    _protocol.sendStartTLS();
                    break;
                }
            }

            // We are now connected
            ImapConfiguration imapCfg(_config);

            if (!_idleProtocol->connected()
                && _protocol.supportsCapability("IDLE")
                && imapCfg.pushEnabled()) {
                _waitingForIdle = true;
                emit updateStatus( tr("Logging in idle connection" ) );
                _idleProtocol->open(imapCfg);
            } else {
                if (!imapCfg.pushEnabled() && _idleProtocol->connected())
                    _idleProtocol->close();
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

        case IMAP_Login:
        {
            // Once we have logged in, we now have the full set of capabilities (either from 
            // a capability command or from an untagged response)

            // Now that we know the capabilities, check for Reference support
            bool supportsReferences(_protocol.capabilities().contains("URLAUTH"));

            QMailAccount account(_config.id());
            if (((account.status() & QMailAccount::CanReferenceExternalData) && !supportsReferences) ||
                (!(account.status() & QMailAccount::CanReferenceExternalData) && supportsReferences)) {
                account.setStatus(QMailAccount::CanReferenceExternalData, supportsReferences);
                if (!QMailStore::instance()->updateAccount(&account)) {
                    qWarning() << "Unable to update account" << account.id() << "to set CanReferenceExternalData";
                }
            }

            _strategyContext->commandTransition(command, status);
            break;
        }

        case IMAP_Logout:
        {
            // Ensure connection is closed on logout
            _protocol.close();
            return;
        }

        case IMAP_Select:
        case IMAP_Examine:
        {
            if (_protocol.mailbox().isSelected()) {
                const ImapMailboxProperties &properties(_protocol.mailbox());

                // See if we have anything to record about this mailbox
                QMailFolder folder(properties.id);

                bool modified(false);
                if ((folder.serverCount() != properties.exists) || (folder.serverUnreadCount() != properties.unseen)) {
                    folder.setServerCount(properties.exists);
                    folder.setServerUnreadCount(properties.unseen);
                    modified = true;

                    // See how this compares to the local mailstore count
                    updateFolderCountStatus(&folder);
                }
                
                if (folder.customField("qmf-uidvalidity") != properties.uidValidity) {
                    folder.setCustomField("qmf-uidvalidity", properties.uidValidity);
                    modified = true;
                }

                if (!properties.noModSeq) {
                    if (folder.customField("qmf-highestmodseq") != properties.highestModSeq) {
                        folder.setCustomField("qmf-highestmodseq", properties.highestModSeq);
                        modified = true;
                    }
                }

                QString supportsForwarded(properties.permanentFlags.contains("$Forwarded", Qt::CaseInsensitive) ? "true" : QString());
                if (folder.customField("qmf-supports-forwarded") != supportsForwarded) {
                    if (supportsForwarded.isEmpty()) {
                        folder.removeCustomField("qmf-supports-forwarded");
                    } else {
                        folder.setCustomField("qmf-supports-forwarded", supportsForwarded);
                    }
                    modified = true;
                }

                if (modified) {
                    if (!QMailStore::instance()->updateFolder(&folder)) {
                        qWarning() << "Unable to update folder" << folder.id() << "to update server counts!";
                    }
                }
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
void ImapClient::mailboxListed(const QString &flags, const QString &path)
{
    QMailFolderId parentId;
    QMailFolderId boxId;

    QMailAccount account(_config.id());

    QString mailboxPath;

    if(_protocol.delimiterUnknown())
        qWarning() << "Delimiter has not yet been discovered, which is essential to know the structure of a mailbox";

    QStringList list = _protocol.flatHierarchy() ? QStringList(path) : path.split(_protocol.delimiter());

    for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
        if (!mailboxPath.isEmpty())
            mailboxPath.append(_protocol.delimiter());
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

            // Is this a special folder?
            ImapConfiguration imapCfg(_config);
            if (!imapCfg.trashFolder().isEmpty() && (imapCfg.trashFolder() == mailboxPath)) {
                folder.setStatus(QMailFolder::Trash | QMailFolder::Incoming, true);
            } else if (!imapCfg.sentFolder().isEmpty() && (imapCfg.sentFolder() == mailboxPath)) {
                folder.setStatus(QMailFolder::Sent | QMailFolder::Outgoing, true);
            } else if (!imapCfg.draftsFolder().isEmpty() && (imapCfg.draftsFolder() == mailboxPath)) {
                folder.setStatus(QMailFolder::Drafts | QMailFolder::Outgoing, true);
            } else if (!imapCfg.junkFolder().isEmpty() && (imapCfg.junkFolder() == mailboxPath)) {
                folder.setStatus(QMailFolder::Junk | QMailFolder::Incoming, true);
            } else {
                folder.setStatus(QMailFolder::Incoming, true);
            }
            if(QString::compare(path, "INBOX", Qt::CaseInsensitive) == 0) {
                //don't let inbox be deleted/renamed
                folder.setStatus(QMailFolder::DeletionPermitted, false);
                folder.setStatus(QMailFolder::RenamePermitted, false);
            } else {
                folder.setStatus(QMailFolder::DeletionPermitted, true);
                folder.setStatus(QMailFolder::RenamePermitted, true);
            }

            if(flags.contains("\\NoInferiors"))
                folder.setStatus(QMailFolder::ChildCreationPermitted, false);
            else
                folder.setStatus(QMailFolder::ChildCreationPermitted, true);

            // The reported flags pertain to the listed folder only
            QString folderFlags;
            if (mailboxPath == path) {
                folderFlags = flags;
            }

            _strategyContext->mailboxListed(folder, folderFlags);

            boxId = mailboxId(mailboxPath);
            parentId = boxId;
        }
    }
}

void ImapClient::messageFetched(QMailMessage& mail)
{
    if (mail.status() & QMailMessage::New) {
        mail.setParentAccountId(_config.id());

        // Some properties are inherited from the folder
        const ImapMailboxProperties &properties(_protocol.mailbox());

        mail.setParentFolderId(properties.id);

        if (properties.status & QMailFolder::Incoming) {
            mail.setStatus(QMailMessage::Incoming, true); 
        }
        if (properties.status & QMailFolder::Outgoing) {
            mail.setStatus(QMailMessage::Outgoing, true); 
        }
        if (properties.status & QMailFolder::Drafts) {
            mail.setStatus(QMailMessage::Draft, true); 
        }
        if (properties.status & QMailFolder::Sent) {
            mail.setStatus(QMailMessage::Sent, true); 
        }
        if (properties.status & QMailFolder::Trash) {
            mail.setStatus(QMailMessage::Trash, true); 
        }
        if (properties.status & QMailFolder::Junk) {
            mail.setStatus(QMailMessage::Junk, true); 
        }
    } else {
        // We need to update the message from the existing data
        QMailMessageMetaData existing(mail.serverUid(), _config.id());
        if (existing.id().isValid()) {
            // Record the status fields that may have been updated
            bool replied(mail.status() & QMailMessage::Replied);
            bool readElsewhere(mail.status() & QMailMessage::ReadElsewhere);
            bool contentAvailable(mail.status() & QMailMessage::ContentAvailable);
            bool partialContentAvailable(mail.status() & QMailMessage::PartialContentAvailable);

            mail.setId(existing.id());
            mail.setParentAccountId(existing.parentAccountId());
            mail.setParentFolderId(existing.parentFolderId());
            mail.setStatus(existing.status());
            mail.setContent(existing.content());
            mail.setReceivedDate(existing.receivedDate());
            mail.setPreviousParentFolderId(existing.previousParentFolderId());
            mail.setInResponseTo(existing.inResponseTo());
            mail.setResponseType(existing.responseType());
            mail.setContentScheme(existing.contentScheme());
            mail.setContentIdentifier(existing.contentIdentifier());
            mail.setCustomFields(existing.customFields());

            // Preserve the status flags determined by the protocol
            mail.setStatus(QMailMessage::Replied, replied);
            mail.setStatus(QMailMessage::ReadElsewhere, readElsewhere);
            if ((mail.status() & QMailMessage::ContentAvailable) || contentAvailable) {
                mail.setStatus(QMailMessage::ContentAvailable, true);
            }
            if ((mail.status() & QMailMessage::PartialContentAvailable) || partialContentAvailable) {
                mail.setStatus(QMailMessage::PartialContentAvailable, true);
            }
        } else {
            qWarning() << "Unable to find existing message for uid:" << mail.serverUid() << "account:" << _config.id();
        }
    }

    _classifier.classifyMessage(mail);

    _strategyContext->messageFetched(mail);
}


void ImapClient::folderCreated(const QString &folder)
{
    mailboxListed(QString(), folder);
    _strategyContext->folderCreated(folder);
}

void ImapClient::folderDeleted(const QMailFolder &folder)
{
    _strategyContext->folderDeleted(folder);
}

void ImapClient::folderRenamed(const QMailFolder &folder, const QString &newPath)
{
    _strategyContext->folderRenamed(folder, newPath);
}

static bool updateParts(QMailMessagePart &part, const QByteArray &bodyData)
{
    static const QByteArray newLine(QMailMessage::CRLF);
    static const QByteArray marker("--");
    static const QByteArray bodyDelimiter(newLine + newLine);

    if (part.multipartType() == QMailMessage::MultipartNone) {
        // The body data is for this part only
        part.setBody(QMailMessageBody::fromData(bodyData, part.contentType(), part.transferEncoding(), QMailMessageBody::AlreadyEncoded));
        part.removeHeaderField("X-qtopiamail-internal-partial-content");
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

class TemporaryFile
{
    enum { BufferSize = 4096 };

    QString _fileName;

public:
    TemporaryFile(const QString &fileName) : _fileName(QMail::tempPath() + QDir::separator() + fileName) {}

    QString fileName() const { return _fileName; }

    bool write(const QMailMessageBody &body)
    {
        QFile file(_fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "Unable to open file for writing:" << _fileName;
            return false;
        }

        // We need to write out the data still encoded - since we're deconstructing
        // server-side data, the meaning of the parameter is reversed...
        QMailMessageBody::EncodingFormat outputFormat(QMailMessageBody::Decoded);

        QDataStream out(&file);
        if (!body.toStream(out, outputFormat)) {
            qWarning() << "Unable to write existing body to file:" << _fileName;
            return false;
        }

        file.close();
        return true;
    }

    static bool copyFileData(QFile &srcFile, QFile &dstFile, qint64 maxLength = -1)
    {
        char buffer[BufferSize];

        while (!srcFile.atEnd() && (maxLength != 0)) {
            qint64 readSize = ((maxLength > 0) ? qMin<qint64>(maxLength, BufferSize) : static_cast<qint64>(BufferSize));
            readSize = srcFile.read(buffer, readSize);
            if (readSize == -1) {
                return false;
            }

            if (maxLength > 0) {
                maxLength -= readSize;
            }
            if (dstFile.write(buffer, readSize) != readSize) {
                return false;
            }
        }

        return true;
    }

    bool appendAndReplace(const QString &fileName)
    {
        {
            QFile existingFile(_fileName);
            QFile dataFile(fileName);

            if (!existingFile.exists()) {
                if (!QFile::copy(fileName, _fileName)) {
                    qWarning() << "Unable to copy - fileName:" << fileName << "_fileName:" << _fileName;
                    return false;
                }
            } else if (existingFile.open(QIODevice::Append)) {
                // On windows, this file will be unwriteable if it is open elsewhere
                if (dataFile.open(QIODevice::ReadOnly)) {
                    if (!copyFileData(dataFile, existingFile)) {
                        qWarning() << "Unable to append data to file:" << _fileName;
                        return false;
                    }
                } else {
                    qWarning() << "Unable to open new data for read:" << fileName;
                    return false;
                }
            } else if (existingFile.open(QIODevice::ReadOnly)) {
                if (dataFile.open(QIODevice::WriteOnly)) {
                    qint64 existingLength = QFileInfo(existingFile).size();
                    qint64 dataLength = QFileInfo(dataFile).size();

                    if (!dataFile.resize(existingLength + dataLength)) {
                        qWarning() << "Unable to resize data file:" << fileName;
                        return false;
                    } else {
                        QFile readDataFile(fileName);
                        if (!readDataFile.open(QIODevice::ReadOnly)) {
                            qWarning() << "Unable to reopen data file for read:" << fileName;
                            return false;
                        }

                        // Copy the data to the end of the file
                        dataFile.seek(existingLength);
                        if (!copyFileData(readDataFile, dataFile, dataLength)) {
                            qWarning() << "Unable to copy existing data in file:" << fileName;
                            return false;
                        }
                    }

                    // Copy the existing data before the new data
                    dataFile.seek(0);
                    if (!copyFileData(existingFile, dataFile, existingLength)) {
                        qWarning() << "Unable to copy existing data to file:" << fileName;
                        return false;
                    }
                } else {
                    qWarning() << "Unable to open new data for write:" << fileName;
                    return false;
                }

                // The complete data is now in the new file
                if (!QFile::remove(_fileName)) {
                    qWarning() << "Unable to remove pre-existing:" << _fileName;
                    return false;
                }

                _fileName = fileName;
                return true;
            } else {
                qWarning() << "Unable to open:" << _fileName;
                return false;
            }
        }

        if (!QFile::remove(fileName)) {
            qWarning() << "Unable to remove:" << fileName;
            return false;
        }
        if (!QFile::rename(_fileName, fileName)) {
            qWarning() << "Unable to rename:" << _fileName << fileName;
            return false;
        }

        _fileName = fileName;
        return true;
    }
};

void ImapClient::dataFetched(const QString &uid, const QString &section, const QString &fileName, int size)
{
    static const QString tempDir = QMail::tempPath() + QDir::separator();

    QMailMessage mail(uid, _config.id());
    if (mail.id().isValid()) {
        if (section.isEmpty()) {
            // This is the body of the message, or a part thereof
            uint existingSize = 0;
            if (mail.hasBody()) {
                existingSize = mail.body().length();

                // Write the existing data to a temporary file
                TemporaryFile tempFile("mail-" + uid + "-body");
                if (!tempFile.write(mail.body())) {
                    qWarning() << "Unable to write existing body to file:" << tempFile.fileName();
                    return;
                }

                if (!tempFile.appendAndReplace(fileName)) {
                    qWarning() << "Unable to append data to existing body file:" << tempFile.fileName();
                    return;
                } else {
                    // The appended content file is now named 'fileName'
                }
            }

            // Set the content into the mail
            mail.setBody(QMailMessageBody::fromFile(fileName, mail.contentType(), mail.transferEncoding(), QMailMessageBody::AlreadyEncoded));
            mail.setStatus(QMailMessage::PartialContentAvailable, true);

            const uint totalSize(existingSize + size);
            if (totalSize >= mail.contentSize()) {
                // We have all the data for this message body
                mail.setStatus(QMailMessage::ContentAvailable, true);
            } 

            // If this message was previously marked read, that is no longer true
            mail.setStatus(QMailMessage::Read, false);
        } else {
            // This is data for a sub-part of the message
            QMailMessagePart::Location partLocation(section);
            if (!partLocation.isValid(false)) {
                qWarning() << "Unable to locate part for invalid section:" << section;
                return;
            } else if (!mail.contains(partLocation)) {
                qWarning() << "Unable to update invalid part for section:" << section;
                return;
            }

            QMailMessagePart &part = mail.partAt(partLocation);

            int existingSize = 0;
            if (part.hasBody()) {
                existingSize = part.body().length();

                // Write the existing data to a temporary file
                TemporaryFile tempFile("mail-" + uid + "-part-" + section);
                if (!tempFile.write(part.body())) {
                    qWarning() << "Unable to write existing body to file:" << tempFile.fileName();
                    return;
                }

                if (!tempFile.appendAndReplace(fileName)) {
                    qWarning() << "Unable to append data to existing body file:" << tempFile.fileName();
                    return;
                } else {
                    // The appended content file is now named 'fileName'
                }
            } 

            if (part.multipartType() == QMailMessage::MultipartNone) {
                // The body data is for this part only
                part.setBody(QMailMessageBody::fromFile(fileName, part.contentType(), part.transferEncoding(), QMailMessageBody::AlreadyEncoded));

                const int totalSize(existingSize + size);
                if (totalSize >= part.contentDisposition().size()) {
                    // We have all the data for this part
                    part.removeHeaderField("X-qtopiamail-internal-partial-content");
                } else {
                    // We only have a portion of the part data
                    part.setHeaderField("X-qtopiamail-internal-partial-content", "true");
                }

                // The file we wrote the content to is detached, and the mailstore can assume ownership
                if (mail.customField("qtopiamail-detached-filename").isEmpty()) {
                    // Only use one detached file at a time
                    mail.setCustomField("qtopiamail-detached-filename", fileName);
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

                // These updates cannot be effected by storing the data file directly
                if (!mail.customField("qtopiamail-detached-filename").isEmpty()) {
                    mail.removeCustomField("qtopiamail-detached-filename");
                }
            }
        }

        _strategyContext->dataFetched(mail, uid, section);
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

void ImapClient::messageCreated(const QMailMessageId &id, const QString &uid)
{
    _strategyContext->messageCreated(id, uid);
}

void ImapClient::downloadSize(const QString &uid, int size)
{
    _strategyContext->downloadSize(uid, size);
}

void ImapClient::urlAuthorized(const QString &url)
{
    _strategyContext->urlAuthorized(url);
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
    deactivateConnection();
    
    // This retrieval may have been asynchronous
    emit allMessagesReceived();

    // Or it may have been requested by a waiting client
    emit retrievalCompleted();
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
            QMailMessageKey::parentFolderId(folderId) &
            QMailMessageKey::status(QMailMessage::Trash));
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

    uint count = QMailStore::instance()->countMessages(folderContent);
    folder->setStatus(QMailFolder::PartialContent, (count < folder->serverCount()));
}

void ImapClient::idling()
{
    if (_waitingForIdle)
        commandCompleted(IMAP_Idle_Continuation, OpOk);
}

void ImapClient::monitor(const QMailFolderIdList &mailboxIds)
{
    static int count(0);
    ++count;
    
    ImapConfiguration imapCfg(_config);
    if (!_protocol.supportsCapability("IDLE")
        || !imapCfg.pushEnabled()) {
        return;
    }
                
    foreach(QMailFolderId id, _monitored.keys()) {
        if (!mailboxIds.contains(id)) {
            IdleProtocol *protocol = _monitored.take(id);
            protocol->close(); // Instead of closing could reuse below in some cases
            delete protocol;
        }
    }
    
    foreach(QMailFolderId id, mailboxIds) {
        if (!_monitored.contains(id)) {
            IdleProtocol *protocol = new IdleProtocol(this, QMailFolder(id));
            protocol->setObjectName(QString("J:%1").arg(count));
            _monitored.insert(id, protocol);
            connect(protocol, SIGNAL(idleNewMailNotification(QMailFolderId)),
                    this, SIGNAL(idleNewMailNotification(QMailFolderId)));
            connect(protocol, SIGNAL(idleFlagsChangedNotification(QMailFolderId)),
                    this, SIGNAL(idleFlagsChangedNotification(QMailFolderId)));
            connect(protocol, SIGNAL(openRequest(IdleProtocol *)),
                    this, SLOT(idleOpenRequested(IdleProtocol *)));
            protocol->open(imapCfg);
        }
    }
}

void ImapClient::idleOpenRequested(IdleProtocol *protocol)
{
    if (protocol == _idleProtocol) {
        if (_protocol.inUse()) { // Setting up new idle connection may be in progress
            const int oneHour = 60*60;
            if (protocol->idleRetryDelay() == oneHour) {
                operationFailed(QMailServiceAction::Status::ErrTimeout, tr("No response"));
                return;
            }
            qMailLog(IMAP) << "IDLE: IMAP IDLE error recovery detected that the primary connection is "
                "busy. Retrying to establish IDLE state in" << protocol->idleRetryDelay()/2 << "seconds.";
            return;
        }
        _protocol.close();
        _idleProtocol->close();
        qMailLog(IMAP) << "IDLE: IMAP IDLE error recovery trying to establish IDLE state now.";
        newConnection();
    } else {
        ImapConfiguration imapCfg(_config);
        if (!_protocol.supportsCapability("IDLE")
            || !imapCfg.pushEnabled()) {
            return;
        }
        protocol->close();
        protocol->open(imapCfg);
    }
}

#include "imapclient.moc"
