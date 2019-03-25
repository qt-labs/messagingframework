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

#include "imapprotocol.h"

#include "imapauthenticator.h"
#include "imapconfiguration.h"
#include "imapstructure.h"
#include "integerregion.h"
#include "imaptransport.h"

#include <QTemporaryFile>
#include <QFileInfo>
#include <QUrl>
#include <qmaillog.h>
#include <private/longstring_p.h>
#include <qmailaccountconfiguration.h>
#include <qmailmessage.h>
#include <qmailmessageserver.h>
#include <qmailnamespace.h>
#include <qmailtransport.h>
#include <qmaildisconnected.h>
#include <qmailcodec.h>

#ifndef QT_NO_SSL
#include <QSslError>
#endif

// Pack both the source mailbox path and the numeric UID into the UID value
// that we store for IMAP messages.  This will allow us find the owner 
// mailbox, even if the UID is present in multiple nested mailboxes.

static QString messageId(const QString& uid)
{
    int index = 0;
    if ((index = uid.lastIndexOf(UID_SEPARATOR)) != -1)
        return uid.mid(index + 1);
    else
        return uid;
}

static QString messageUid(const QMailFolderId &folderId, const QString &id)
{
    return QString::number(folderId.toULongLong()) + UID_SEPARATOR + id;
}

static QString extractUid(const QString &field, const QMailFolderId &folderId)
{
    QRegExp uidFormat("UID *(\\d+)");
    uidFormat.setCaseSensitivity(Qt::CaseInsensitive);
    if (uidFormat.indexIn(field) != -1) {
        return messageUid(folderId, uidFormat.cap(1));
    }

    return QString();
}

static QDateTime extractDate(const QString& field)
{
    QRegExp dateFormat("INTERNALDATE *\"([^\"]*)\"");
    dateFormat.setCaseSensitivity(Qt::CaseInsensitive);
    if (dateFormat.indexIn(field) != -1) {
        QString date(dateFormat.cap(1));

        QRegExp format("(\\d+)-(\\w{3})-(\\d{4}) (\\d{2}):(\\d{2}):(\\d{2}) ([+-])(\\d{2})(\\d{2})");
        if (format.indexIn(date) != -1) {
            static const QString Months("janfebmaraprmayjunjulaugsepoctnovdec");
            int month = (Months.indexOf(format.cap(2).toLower()) + 3) / 3;

            QDate dateComponent(format.cap(3).toInt(), month, format.cap(1).toInt());
            QTime timeComponent(format.cap(4).toInt(), format.cap(5).toInt(), format.cap(6).toInt());
            int offset = (format.cap(8).toInt() * 3600) + (format.cap(9).toInt() * 60);

            QDateTime timeStamp(dateComponent, timeComponent, Qt::UTC);
            timeStamp = timeStamp.addSecs(offset * (format.cap(7)[0] == '-' ? 1 : -1));
            return timeStamp;
        }
    }

    return QDateTime();
}

static uint extractSize(const QString& field)
{
    QRegExp sizeFormat("RFC822\\.SIZE *(\\d+)");
    sizeFormat.setCaseSensitivity(Qt::CaseInsensitive);
    if (sizeFormat.indexIn(field) != -1) {
        return sizeFormat.cap(1).toUInt();
    }

    return 0;
}

static QStringList extractStructure(const QString& field)
{
    return getMessageStructure(field);
}

static bool parseFlags(const QString& field, MessageFlags& flags)
{
    QRegExp pattern("FLAGS *\\((.*)\\)");
    pattern.setMinimal(true);
    pattern.setCaseSensitivity(Qt::CaseInsensitive);

    if (pattern.indexIn(field) == -1)
        return false;

    QString messageFlags = pattern.cap(1).toLower();

    flags = 0;
    if (messageFlags.indexOf("\\seen") != -1)
        flags |= MFlag_Seen;
    if (messageFlags.indexOf("\\answered") != -1)
        flags |= MFlag_Answered;
    if (messageFlags.indexOf("\\flagged") != -1)
        flags |= MFlag_Flagged;
    if (messageFlags.indexOf("\\deleted") != -1)
        flags |= MFlag_Deleted;
    if (messageFlags.indexOf("\\draft") != -1)
        flags |= MFlag_Draft;
    if (messageFlags.indexOf("\\recent") != -1)
        flags |= MFlag_Recent;
    if (messageFlags.indexOf("$forwarded") != -1)
        flags |= MFlag_Forwarded;

    return true;
}

static int indexOfWithEscape(const QString &str, QChar c, int from, const QString &ignoreEscape)
{
    int index = from;
    int pos = str.indexOf(c, index, Qt::CaseInsensitive);
    if (ignoreEscape.isEmpty()) {
        return pos;
    }
    const int ignoreLength = ignoreEscape.length();
    while (pos + 1 >= ignoreLength) {
        if (str.mid(pos - ignoreLength + 1, ignoreLength) == ignoreEscape) {
            index = pos + 1;
            pos = str.indexOf(c, index, Qt::CaseInsensitive);
            continue;
        } else {
            break;
        }
    }

    return pos;
}

static QString token( QString str, QChar c1, QChar c2, int *index, const QString &ignoreEscape = QString())
{
    int start, stop;

    // The strings we're tokenizing use CRLF as the line delimiters - assume that the
    // caller considers the sequence to be atomic.
    if (c1 == QMailMessage::CarriageReturn)
        c1 = QMailMessage::LineFeed;
    start = indexOfWithEscape(str, c1, *index, ignoreEscape);
    if (start == -1)
        return QString();

    if (c2 == QMailMessage::CarriageReturn)
        c2 = QMailMessage::LineFeed;
    stop = indexOfWithEscape(str, c2, ++start, ignoreEscape);
    if (stop == -1)
        return QString();

    // Exclude the CR if necessary
    if (stop && (str[stop-1] == QMailMessage::CarriageReturn))
        --stop;

    // Bypass the LF if necessary
    *index = stop + 1;

    return str.mid( start, stop - start );
}

static QList<uint> sequenceUids(const QString &sequence)
{
    QList<uint> uids;

    foreach (const QString &uid, sequence.split(",")) {
        int index = uid.indexOf(":");
        if (index != -1) {
            uint first = uid.left(index).toUInt();
            uint last = uid.mid(index + 1).toUInt();
            for ( ; first <= last; ++first)
                uids.append(first);
        } else {
            uids.append(uid.toUInt());
        }
    }

    return uids;
}

static QString searchFlagsToString(MessageFlags flags)
{
    QStringList result;

    if (flags != 0) {
        if (flags & MFlag_Recent)
            result.append("RECENT");
        if (flags & MFlag_Deleted)
            result.append("DELETED");
        if (flags & MFlag_Answered)
            result.append("ANSWERED");
        if (flags & MFlag_Flagged)
            result.append("FLAGGED");
        if (flags & MFlag_Seen)
            result.append("SEEN");
        if (flags & MFlag_Unseen)
            result.append("UNSEEN");
        if (flags & MFlag_Draft)
            result.append("DRAFT");
        if (flags & MFlag_Forwarded)
            result.append("$FORWARDED");
    }

    return result.join(QString(' '));
}

static QString messageFlagsToString(MessageFlags flags)
{
    QStringList result;

    // Note that \Recent flag is ignored as only the server is allowed to modify that flag 
    if (flags != 0) {
        if (flags & MFlag_Deleted)
            result.append("\\Deleted");
        if (flags & MFlag_Answered)
            result.append("\\Answered");
        if (flags & MFlag_Flagged)
            result.append("\\Flagged");
        if (flags & MFlag_Seen)
            result.append("\\Seen");
        if (flags & MFlag_Draft)
            result.append("\\Draft");
        if (flags & MFlag_Forwarded)
            result.append("$Forwarded");
    }

    return result.join(QString(' '));
}

static MessageFlags flagsForMessage(const QMailMessageMetaData &metaData)
{
    MessageFlags result(0);

    if (metaData.status() & (QMailMessage::Read | QMailMessage::ReadElsewhere | QMailMessage::Outgoing)) {
        result |= MFlag_Seen;
    }
    if (metaData.status() & QMailMessage::Important) {
        result |= MFlag_Flagged;
    }
    if (metaData.status() & (QMailMessage::Replied | QMailMessage::RepliedAll)) {
        result |= MFlag_Answered;
    }
    if (metaData.status() & QMailMessage::Forwarded) {
        result |= MFlag_Forwarded;
    }
    if (metaData.status() & QMailMessage::Draft) {
        result |= MFlag_Draft;
    }

    return result;
}


/* Begin state design pattern related classes */

class ImapState;

class ImapContext
{
public:
    ImapContext(ImapProtocol *protocol) { mProtocol = protocol; }
    virtual ~ImapContext() {}

    void continuation(ImapCommand c, const QString &s) { mProtocol->continuation(c, s); }
    void operationCompleted(ImapCommand c, OperationStatus s) { mProtocol->operationCompleted(c, s); }

    virtual QString sendCommand(const QString &cmd) { return mProtocol->sendCommand(cmd); }
    virtual QString sendCommandLiteral(const QString &cmd, uint length) { return mProtocol->sendCommandLiteral(cmd, length); }
    virtual void sendData(const QString &data, bool maskDebug = false) { mProtocol->sendData(data, maskDebug); }
    virtual void sendDataLiteral(const QString &data, uint length) { mProtocol->sendDataLiteral(data, length); }

    ImapProtocol *protocol() { return mProtocol; }
    const ImapMailboxProperties &mailbox() { return mProtocol->mailbox(); }

    LongStream &buffer() { return mProtocol->_stream; }
#ifndef QT_NO_SSL
    void switchToEncrypted() { mProtocol->_transport->switchToEncrypted(); mProtocol->clearResponse(); }
#endif
    bool literalResponseCompleted() { return (mProtocol->literalDataRemaining() == 0); }

    // Update the protocol's mailbox properties
    void setMailbox(const QMailFolder &mailbox) { mProtocol->_mailbox = ImapMailboxProperties(mailbox); }
    void setExists(quint32 n) { mProtocol->_mailbox.exists = n; emit mProtocol->exists(n); }
    quint32 exists() { return mProtocol->_mailbox.exists; }
    void setRecent(quint32 n) { mProtocol->_mailbox.recent = n; emit mProtocol->recent(n); }
    void setUnseen(quint32 n) { mProtocol->_mailbox.unseen = n; }
    void setUidValidity(const QString &validity) { mProtocol->_mailbox.uidValidity = validity; emit mProtocol->uidValidity(validity); }
    void setUidNext(quint32 n) { mProtocol->_mailbox.uidNext = n; }
    void setFlags(const QString &flags) { mProtocol->_mailbox.flags = flags; emit mProtocol->flags(flags); }
    void setUidList(const QStringList &uidList) { mProtocol->_mailbox.uidList = uidList; }
    void setSearchCount(uint count) { mProtocol->_mailbox.searchCount = count; }
    void setMsnList(const QList<uint> &msnList) { mProtocol->_mailbox.msnList = msnList; }
    void setHighestModSeq(const QString &seq) { mProtocol->_mailbox.highestModSeq = seq; mProtocol->_mailbox.noModSeq = false; emit mProtocol->highestModSeq(seq); }
    void setNoModSeq() { mProtocol->_mailbox.noModSeq = true; emit mProtocol->noModSeq(); }
    void setPermanentFlags(const QStringList &flags) { mProtocol->_mailbox.permanentFlags = flags; }
    void setVanished(const QString &vanished) { mProtocol->_mailbox.vanished = vanished; }
    void setChanges(const QList<FlagChange> &changes) { mProtocol->_mailbox.flagChanges = changes; }

    void createMail(const QString& uid, const QDateTime &timeStamp, int size, uint flags, const QString &file, const QStringList& structure) { mProtocol->createMail(uid, timeStamp, size, flags, file, structure); }
    void createPart(const QString& uid, const QString &section, const QString &file, int size) { mProtocol->createPart(uid, section, file, size); }
    void createPartHeader(const QString& uid, const QString &section, const QString &file, int size) { mProtocol->createPartHeader(uid, section, file, size); }

private:
    ImapProtocol *mProtocol;
};


class ImapState : public QObject
{
    Q_OBJECT
    
public:
    ImapState(ImapCommand c, const QString &name)
        : mCommand(c), mName(name), mStatus(OpPending) {}

    virtual ~ImapState() {}

    virtual void init() { mStatus = OpPending; mTag.clear(); }

    virtual QString transmit(ImapContext *) { return QString(); }
    virtual void enter(ImapContext *) {}
    virtual void leave(ImapContext *) { init(); }
    
    virtual bool permitsPipelining() const { return false; }

    virtual bool continuationResponse(ImapContext *c, const QString &line);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
    virtual void taggedResponse(ImapContext *c, const QString &line);
    virtual void literalResponse(ImapContext *c, const QString &line);
    virtual bool appendLiteralData(ImapContext *c, const QString &preceding);

    virtual QString error(const QString &line);
    void log(const QString &note);

    virtual QString tag() { return mTag; }
    virtual void setTag(const QString &tag) { mTag = tag; }

    ImapCommand command() const { return mCommand; }
    void setCommand(ImapCommand c) { mCommand = c; }

    OperationStatus status() const { return mStatus; }
    void setStatus(OperationStatus s) { mStatus = s; }

private:
    ImapCommand mCommand;
    QString mName;
    OperationStatus mStatus;
    QString mTag;
};

bool ImapState::continuationResponse(ImapContext *, const QString &line)
{
    qWarning() << "Unexpected continuation response!" << line;
    return false;
}

void ImapState::untaggedResponse(ImapContext *c, const QString &line)
{
    int index = line.indexOf("[ALERT]", Qt::CaseInsensitive);
    if (index != -1) {
        qWarning() << line.mid(index).toLatin1();
    } else if (line.indexOf("[CAPABILITY", 0) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        QStringList capabilities = temp.mid(12).trimmed().split(' ', QString::SkipEmptyParts);
        c->protocol()->setCapabilities(capabilities);
    } else if (line.indexOf("* CAPABILITY ", 0) != -1) {
        QStringList capabilities = line.mid(13).trimmed().split(' ', QString::SkipEmptyParts);
        c->protocol()->setCapabilities(capabilities);
    }

    c->buffer().append(line);
}

void ImapState::taggedResponse(ImapContext *c, const QString &line)
{
    int index = line.indexOf("[ALERT]", Qt::CaseInsensitive);
    if (index != -1)
        qWarning() << line.mid(index).toLatin1();

    c->operationCompleted(mCommand, mStatus);
}

void ImapState::literalResponse(ImapContext *, const QString &)
{
}

bool ImapState::appendLiteralData(ImapContext *, const QString &)
{
    return true;
}

QString ImapState::error(const QString &line) 
{ 
    return line;
}

void ImapState::log(const QString &note)
{
    QString result;
    switch (mStatus) {
    case OpPending: 
        result = "OpPending";
        break;
    case OpFailed: 
        result = "OpFailed";
        break;
    case OpOk: 
        result = "OpOk";
        break;
    case OpNo: 
        result = "OpNo";
        break;
    case OpBad: 
        result = "OpBad";
        break;
    }
    qMailLog(MessagingState) << note << mName << result;
}


// IMAP concrete states

class UnconnectedState : public ImapState
{
    Q_OBJECT

public:
    UnconnectedState() : ImapState(IMAP_Unconnected, "Unconnected") { setStatus(OpOk); }
    virtual void init() { ImapState::init(); setStatus(OpOk); }
};


class InitState : public ImapState
{
    Q_OBJECT

public:
    InitState() : ImapState(IMAP_Init, "Init") {}

    virtual void untaggedResponse(ImapContext *c, const QString &line);
};

void InitState::untaggedResponse(ImapContext *c, const QString &line)
{
    ImapState::untaggedResponse(c, line);

    // We're only waiting for an untagged response here
    setStatus(OpOk);
    c->operationCompleted(command(), status());
}


class CapabilityState : public ImapState
{
    Q_OBJECT

public:
    CapabilityState() : ImapState(IMAP_Capability, "Capability") {}

    virtual QString transmit(ImapContext *c);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
};

QString CapabilityState::transmit(ImapContext *c)
{
    return c->sendCommand("CAPABILITY");
}

void CapabilityState::untaggedResponse(ImapContext *c, const QString &line)
{
    QStringList capabilities;
    if (line.startsWith(QLatin1String("* CAPABILITY"))) {
        capabilities = line.mid(12).trimmed().split(' ', QString::SkipEmptyParts);
        c->protocol()->setCapabilities(capabilities);
    } else {
        ImapState::untaggedResponse(c, line);
    }
}


class StartTlsState : public ImapState
{
    Q_OBJECT

public:
    StartTlsState() : ImapState(IMAP_StartTLS, "StartTLS") {}

    virtual QString transmit(ImapContext *c);
    virtual void taggedResponse(ImapContext *c, const QString &line);
};

QString StartTlsState::transmit(ImapContext *c)
{
    return c->sendCommand("STARTTLS");
}

void StartTlsState::taggedResponse(ImapContext *c, const QString &)
{
#ifndef QT_NO_SSL
    // Switch to encrypted comms mode
    c->switchToEncrypted();
#else
    Q_UNUSED(c)
#endif
}


class LoginState : public ImapState
{
    Q_OBJECT

public:
    LoginState() : ImapState(IMAP_Login, "Login") { LoginState::init(); }

    void setConfiguration(const QMailAccountConfiguration &config, const QStringList &capabilities);

    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual bool continuationResponse(ImapContext *c, const QString &line);
    virtual void taggedResponse(ImapContext *c, const QString &line);

private:
    QMailAccountConfiguration _config;
    QStringList _capabilities;
};

void LoginState::setConfiguration(const QMailAccountConfiguration &config, const QStringList &capabilities)
{
    _config = config;
    _capabilities = capabilities;
}

void LoginState::init()
{
    ImapState::init();
    _config = QMailAccountConfiguration();
    _capabilities = QStringList();
}

QString LoginState::transmit(ImapContext *c)
{
    return c->sendCommand(ImapAuthenticator::getAuthentication(_config.serviceConfiguration("imap4"), _capabilities));
}

bool LoginState::continuationResponse(ImapContext *c, const QString &received)
{
    // The server input is Base64 encoded
    QByteArray challenge = QByteArray::fromBase64(received.toLatin1());
    QByteArray response(ImapAuthenticator::getResponse(_config.serviceConfiguration("imap4"), challenge));

    if (!response.isEmpty()) {
        c->sendData(response.toBase64(), true);
    }

    return false;
}

void LoginState::taggedResponse(ImapContext *c, const QString &line)
{
    if (line.indexOf("[CAPABILITY", Qt::CaseInsensitive) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        QStringList capabilities = temp.mid(12).trimmed().split(' ', QString::SkipEmptyParts);
        c->protocol()->setCapabilities(capabilities);
    }

    c->protocol()->setAuthenticated(true);
    ImapState::taggedResponse(c, line);
}


class LogoutState : public ImapState
{
    Q_OBJECT

public:
    LogoutState() : ImapState(IMAP_Logout, "Logout") {}

    virtual QString transmit(ImapContext *c);
    virtual void taggedResponse(ImapContext *c, const QString &line);
};

QString LogoutState::transmit(ImapContext *c)
{
    return c->sendCommand("LOGOUT");
}

void LogoutState::taggedResponse(ImapContext *c, const QString &line)
{
    if (status() == OpOk) {
        c->protocol()->setAuthenticated(false);
        c->protocol()->close();
        c->operationCompleted(command(), OpOk);
    } else {
        ImapState::taggedResponse(c, line);
    }
}

class CreateState : public ImapState
{
    Q_OBJECT

public:
    CreateState() : ImapState(IMAP_Create, "Create") {}

    void setMailbox(const QMailFolderId &parentFolderId, const QString &name);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual void taggedResponse(ImapContext *c, const QString &line);
    virtual QString error(const QString &line);
signals:
    void folderCreated(const QString &name, bool success);

private:
    QString makePath(ImapContext *c, const QMailFolderId &parent, const QString &name);
    QList<QPair<QMailFolderId, QString> > _mailboxes;
};

void CreateState::setMailbox(const QMailFolderId &parentFolderId, const QString &name)
{
    _mailboxes.append(qMakePair(parentFolderId, name));
}

void CreateState::init()
{
    _mailboxes.clear();
    ImapState::init();
}

QString CreateState::transmit(ImapContext *c)
{  
    const QMailFolderId &parent = _mailboxes.last().first;
    const QString &name = _mailboxes.last().second;

    if(parent.isValid() && c->protocol()->delimiterUnknown()) {
        // We are waiting on delim to create
        return QString();
    }

    if (name.contains(c->protocol()->delimiter())) {
        qWarning() << "Unsupported: folder name contains IMAP delimiter" << name << c->protocol()->delimiter();
        emit folderCreated(makePath(c, parent, name), false);
        c->operationCompleted(command(), OpFailed);
        return QString();
    }

    QString path = makePath(c, parent, name);

    QString cmd("CREATE " + ImapProtocol::quoteString(path));
    return c->sendCommand(cmd);
}

void CreateState::leave(ImapContext *)
{
    ImapState::init();
    _mailboxes.removeFirst();
}

void CreateState::taggedResponse(ImapContext *c, const QString &line)
{
    emit folderCreated(makePath(c, _mailboxes.first().first, _mailboxes.first().second), status() == OpOk);
    ImapState::taggedResponse(c, line);
}

QString CreateState::error(const QString &line)
{
    qWarning() << "CreateState::error:" << line;
    emit folderCreated(_mailboxes.first().second, false);
    return ImapState::error(line);
}

QString CreateState::makePath(ImapContext *c, const QMailFolderId &parent, const QString &name)
{
    QString path;

    if(parent.isValid()) {
        if(!c->protocol()->delimiterUnknown())
            path = QMailFolder(parent).path() + c->protocol()->delimiter();
        else
            qWarning() << "Cannot create a child folder, without a delimiter";

    }
    return (path + QMailCodec::encodeModifiedUtf7(name));
}

class DeleteState : public ImapState
{
    Q_OBJECT

public:
    DeleteState() : ImapState(IMAP_Delete, "Delete") {}

    void setMailbox(QMailFolder mailbox);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual void taggedResponse(ImapContext *c, const QString &line);
    virtual QString error(const QString &line);
signals:
    void folderDeleted(const QMailFolder &name, bool success);

private:
    QList<QMailFolder> _mailboxList;
};

void DeleteState::setMailbox(QMailFolder mailbox)
{
    _mailboxList.append(mailbox);
}

void DeleteState::init()
{
    _mailboxList.clear();
    ImapState::init();
}

QString DeleteState::transmit(ImapContext *c)
{
    QString cmd("DELETE " + ImapProtocol::quoteString(_mailboxList.last().path()));
    return c->sendCommand(cmd);
}

void DeleteState::leave(ImapContext *)
{
    ImapState::init();
    _mailboxList.removeFirst();
}

void DeleteState::taggedResponse(ImapContext *c, const QString &line)
{
    emit folderDeleted(_mailboxList.first(), status() == OpOk);
    ImapState::taggedResponse(c, line);
}

QString DeleteState::error(const QString &line)
{
    qWarning() << "DeleteState::error:" << line;
    emit folderDeleted(_mailboxList.first(), false);
    return ImapState::error(line);
}

class RenameState : public ImapState
{
    Q_OBJECT

public:
    RenameState() : ImapState(IMAP_Rename, "Rename") {}

    void setNewMailboxName(const QMailFolder &mailbox, const QString &name);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual void taggedResponse(ImapContext *c, const QString &line);
    virtual QString error(const QString &line);
signals:
    void folderRenamed(const QMailFolder &folder, const QString &newPath, bool success);

private:
    QString buildNewPath(ImapContext *c , const QMailFolder &folder, QString &newName);
    QList<QPair<QMailFolder, QString> > _mailboxNames;
};

void RenameState::setNewMailboxName(const QMailFolder &mailbox, const QString &newName)
{
    _mailboxNames.append(qMakePair(mailbox, newName));
}

void RenameState::init()
{
    _mailboxNames.clear();
    ImapState::init();
}

QString RenameState::transmit(ImapContext *c)
{
    if(c->protocol()->delimiterUnknown()) {
        // We are waiting on delim to create
        return QString();
    }

    QString from = _mailboxNames.last().first.path();
    QString to =  buildNewPath(c, _mailboxNames.last().first, _mailboxNames.last().second);
    if (_mailboxNames.last().second.contains(c->protocol()->delimiter())) {
        qWarning() << "Unsupported: new name contains IMAP delimiter" << _mailboxNames.last().second
                   << c->protocol()->delimiter();
        emit folderRenamed(from, to, false);
        c->operationCompleted(command(), OpFailed);
        return QString();
    }

    QString cmd(QString("RENAME %1 %2").arg(ImapProtocol::quoteString(from)).arg( ImapProtocol::quoteString(to)));
    return c->sendCommand(cmd);
}

void RenameState::leave(ImapContext *)
{
    ImapState::init();
    _mailboxNames.removeFirst();
}

void RenameState::taggedResponse(ImapContext *c, const QString &line)
{
    QString path = buildNewPath(c, _mailboxNames.first().first, _mailboxNames.first().second);
    emit folderRenamed(_mailboxNames.first().first, path, status() == OpOk);
    ImapState::taggedResponse(c, line);
}

QString RenameState::error(const QString &line)
{
    qWarning() << "RenameState::error:" << line;
    emit folderRenamed(_mailboxNames.first().first, _mailboxNames.first().second, false);
    return ImapState::error(line);
}

QString RenameState::buildNewPath(ImapContext *c , const QMailFolder &folder, QString &newName)
{
    QString path;
    QString encodedNewName = QMailCodec::encodeModifiedUtf7(newName);
    if(c->protocol()->flatHierarchy() || folder.path().count(c->protocol()->delimiter()) == 0)
        path = encodedNewName;
    else
        path = folder.path().section(c->protocol()->delimiter(), 0, -2) + c->protocol()->delimiter() + encodedNewName;
    return path;
}

class MoveState : public ImapState
{
    Q_OBJECT

public:
    MoveState() : ImapState(IMAP_Move, "Move") {}

    void setNewMailboxParent(const QMailFolder &mailbox, const QMailFolderId &newParentId);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual void taggedResponse(ImapContext *c, const QString &line);
    virtual QString error(const QString &line);
signals:
    void folderMoved(const QMailFolder &folder, const QString &newPath,
                     const QMailFolderId &newParentId, bool success);

private:
    QString buildNewPath(ImapContext *c, const QMailFolder &folder, const QMailFolderId &newParentId);
    QList<QPair<QMailFolder, QMailFolderId> > _mailboxParents;
};

void MoveState::setNewMailboxParent(const QMailFolder &mailbox, const QMailFolderId &newParentId)
{
    _mailboxParents.append(qMakePair(mailbox, newParentId));
}

void MoveState::init()
{
    _mailboxParents.clear();
    ImapState::init();
}

QString MoveState::transmit(ImapContext *c)
{
    if (c->protocol()->delimiterUnknown()) {
        // We are waiting on delim to move
        return QString();
    }

    QString from = _mailboxParents.last().first.path();
    QString to =  buildNewPath(c, _mailboxParents.last().first, _mailboxParents.last().second);
    QString cmd(QString("RENAME %1 %2").arg(ImapProtocol::quoteString(from)).arg( ImapProtocol::quoteString(to)));
    return c->sendCommand(cmd);
}

void MoveState::leave(ImapContext *)
{
    ImapState::init();
    _mailboxParents.removeFirst();
}

void MoveState::taggedResponse(ImapContext *c, const QString &line)
{
    QString path = buildNewPath(c, _mailboxParents.first().first, _mailboxParents.first().second);
    emit folderMoved(_mailboxParents.first().first, path, _mailboxParents.first().second, status() == OpOk);
    ImapState::taggedResponse(c, line);
}

QString MoveState::error(const QString &line)
{
    qWarning() << "MoveState::error:" << line;
    emit folderMoved(_mailboxParents.first().first, QString(), _mailboxParents.first().second, false);
    return ImapState::error(line);
}

QString MoveState::buildNewPath(ImapContext *c, const QMailFolder &folder, const QMailFolderId &newParentId)
{
    QString path;
    if (c->protocol()->flatHierarchy() || c->protocol()->delimiter() == 0) {
        path = folder.path();
    } else if (newParentId.isValid()) {
        QMailFolder parentFolder(newParentId);
        path = parentFolder.path() + c->protocol()->delimiter() + folder.path().section(c->protocol()->delimiter(), -1);
    } else {
        path = folder.path().section(c->protocol()->delimiter(), -1);
    }
    return path;
}

class ListState : public ImapState
{
    Q_OBJECT
    
public:
    ListState() : ImapState(IMAP_List, "List") { ListState::init(); }

    void setParameters(const QString &reference, const QString &mailbox, bool xlist = false);
    void setDiscoverDelimiter();

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
    virtual void taggedResponse(ImapContext *c, const QString &line);
    
signals:
    void mailboxListed(const QString &flags, const QString &path);

private:
    struct ListParameters
    {
        ListParameters() : _xlist(false) {}
        
        QString _reference;
        QString _mailbox;
        bool _xlist;
    };
    
    // The list of reference/mailbox pairs we're listing (via multiple commands), in order
    QList<ListParameters> _parameters;
};

void ListState::setParameters(const QString &reference, const QString &mailbox, bool xlist)
{
    ListParameters params;
    params._reference = reference;
    params._mailbox = mailbox;
    params._xlist = xlist;

    _parameters.append(params);
}

void ListState::setDiscoverDelimiter()
{
    setParameters(QString(), QString());
}

void ListState::init()
{
    ImapState::init();
    _parameters.clear();
}

QString ListState::transmit(ImapContext *c)
{
    ListParameters &params(_parameters.last());

    if (!params._reference.isEmpty()) {
        if (c->protocol()->delimiterUnknown()) {
            //Don't do anything, we're waiting on our delimiter.
            return QString();
        }
    }

    QString reference = params._reference;
    QString mailbox = params._mailbox;
    if (!reference.isEmpty())
        reference.append(c->protocol()->delimiter());
    reference = ImapProtocol::quoteString(reference);
    mailbox = ImapProtocol::quoteString(mailbox);

    QString command("LIST");
    if (params._xlist)
        command = "XLIST";
    return c->sendCommand(QString("%1 %2 %3").arg(command).arg(reference).arg(mailbox));
}

void ListState::leave(ImapContext *)
{
    ImapState::init();
    _parameters.removeFirst();
}

void ListState::untaggedResponse(ImapContext *c, const QString &line)
{
    QString str;
    bool isXList(false);
    if (line.startsWith(QLatin1String("* LIST"))) {
        str = line.mid(7);
    } else if (line.startsWith(QLatin1String("* XLIST"))) {
        isXList = true;
        str = line.mid(8);
    } else {
        ImapState::untaggedResponse(c, line);
        return;
    }

    QString flags, path, delimiter;
    int pos, index = 0;

    flags = token(str, '(', ')', &index);

    delimiter = token(str, ' ', ' ', &index);
    if(c->protocol()->delimiterUnknown()) //only figure it out precisely if needed
    {
        if(delimiter == "NIL") {
            c->protocol()->setFlatHierarchy(true);
        } else {
            pos = 0;
            if (!token(delimiter, '"', '"', &pos).isNull()) {
                pos = 0;
                delimiter = token(delimiter, '"', '"', &pos);
            }
            if(delimiter.length() != 1)
                qWarning() << "Delimiter length is" << delimiter.length() << "while should only be 1 character";
            c->protocol()->setDelimiter(*delimiter.begin());
        }
    }

    index--;    //to point back to previous ' ' so we can find it with next search
    path = token(str, ' ', '\n', &index).trimmed();
    pos = 0;
    if (!token(path, '"', '"', &pos, "\\\"").isNull()) {
        pos = 0;
        path = token(path, '"', '"', &pos, "\\\"");
    }

    if (!path.isEmpty()) {
        // Translate the inbox folder to force the path "INBOX" as returned by normal LIST command
        if (isXList && flags.indexOf("Inbox", 0, Qt::CaseInsensitive) != -1) {
            path = "INBOX";
        }
        emit mailboxListed(flags, ImapProtocol::unescapeFolderPath(path));
    }
}

void ListState::taggedResponse(ImapContext *c, const QString &line)
{
    ListParameters &params(_parameters.first());

    if (params._reference.isNull() && params._mailbox.isNull()) {
        // This was a delimiter discovery request - don't report it to the client
    } else {
        ImapState::taggedResponse(c, line);
    }
}


class GenUrlAuthState : public ImapState
{
    Q_OBJECT

public:
    GenUrlAuthState() : ImapState(IMAP_GenUrlAuth, "GenUrlAuth") {}

    void setUrl(const QString &url, const QString &mechanism);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual void untaggedResponse(ImapContext *c, const QString &line);

signals:
    void urlAuthorized(const QString &url);

private:
    QList<QPair<QString, QString> > _parameters;
};

void GenUrlAuthState::setUrl(const QString &url, const QString &mechanism)
{
    _parameters.append(qMakePair(url, (mechanism.isEmpty() ? "INTERNAL" : mechanism)));
}

void GenUrlAuthState::init()
{
    ImapState::init();
    _parameters.clear();
}

QString GenUrlAuthState::transmit(ImapContext *c)
{
    const QPair<QString, QString> &params(_parameters.last());

    return c->sendCommand("GENURLAUTH \"" + params.first + "\" " + params.second);
}

void GenUrlAuthState::leave(ImapContext *)
{
    ImapState::init();
    _parameters.removeFirst();
}

void GenUrlAuthState::untaggedResponse(ImapContext *c, const QString &line)
{
    if (!line.startsWith(QLatin1String("* GENURLAUTH"))) {
        ImapState::untaggedResponse(c, line);
        return;
    }

    emit urlAuthorized(QMail::unquoteString(line.mid(13).trimmed()));
}


class AppendState : public ImapState
{
    Q_OBJECT

public:
    AppendState() : ImapState(IMAP_Append, "Append") {}

    void setParameters(const QMailFolder &folder, const QMailMessageId &messageId);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual bool continuationResponse(ImapContext *c, const QString &line);
    virtual void taggedResponse(ImapContext *c, const QString &line);

signals:
    void messageCreated(const QMailMessageId&, const QString&);

private:
    struct AppendParameters
    {
        AppendParameters() : mCatenate(false) {}

        QMailFolder mDestination;
        QMailMessageId mMessageId;
        QList<QPair<QByteArray, uint> > mData;
        bool mCatenate;
    };

    QList<AppendParameters> mParameters;
};

void AppendState::setParameters(const QMailFolder &folder, const QMailMessageId &messageId)
{
    AppendParameters params;
    params.mDestination = folder;
    params.mMessageId = messageId;

    mParameters.append(params);
}

void AppendState::init()
{
    ImapState::init();
    mParameters.clear();
}

static QList<QPair<QByteArray, uint> > dataSequence(QList<QMailMessage::MessageChunk> &chunks)
{
    QList<QPair<QByteArray, uint> > result;
    QByteArray sequence;

    while (!chunks.isEmpty()) {
        const QMailMessage::MessageChunk &chunk(chunks.first());

        if (chunk.first == QMailMessage::Text) {
            sequence.append(" TEXT");

            // We can't send any more in this sequence
            uint literalLength = chunk.second.length();
            result.append(qMakePair(sequence, literalLength));

            sequence.clear();
            sequence.append(chunk.second);
        } else if (chunk.first == QMailMessage::Reference) {
            sequence.append(" URL ");
            sequence.append(chunk.second);
        }

        // We're finished with this chunk
        chunks.removeFirst();
    }

    if (!sequence.isEmpty()) {
        result.append(qMakePair(sequence, 0u));
    }

    return result;
}

QString AppendState::transmit(ImapContext *c)
{
    AppendParameters &params(mParameters.last());

    QMailMessage message(params.mMessageId);

    QByteArray cmd;
    QString cmdString("APPEND ");;
    cmdString += ImapProtocol::quoteString(params.mDestination.path());
    cmdString += " (";
    cmdString += messageFlagsToString(flagsForMessage(message));
    cmdString += ") \"";
    cmdString += message.date().toString(QMailTimeStamp::Rfc3501);
    cmdString += "\"";
    cmd = cmdString.toLatin1();


    uint length = 0;

    if (c->protocol()->capabilities().contains("CATENATE")) {
        QList<QMailMessage::MessageChunk> chunks(message.toRfc2822Chunks(QMailMessage::TransmissionFormat));
        if ((chunks.count() == 1) && (chunks.first().first == QMailMessage::Text)) {
            // This is a single piece of text - no benefit to using catenate
            params.mData.append(qMakePair(chunks.first().second, 0u));
            length = params.mData.first().first.length();
        } else {
            params.mData = dataSequence(chunks);
            params.mCatenate = true;

            // Skip the leading space in the first element
            cmd.append(" CATENATE (");
            cmd.append(params.mData.first().first.mid(1));
            length = params.mData.first().second;

            params.mData.removeFirst();
            if (params.mData.isEmpty()) {
                // We have no literal data to send
                cmd.append(")");
                return c->sendCommand(cmd);
            }
        }
    } else {
        params.mData.append(qMakePair(message.toRfc2822(QMailMessage::TransmissionFormat), 0u));
        length = params.mData.first().first.length();
    }

    return c->sendCommandLiteral(cmd, length);
}

void AppendState::leave(ImapContext *)
{
    ImapState::init();
    mParameters.removeFirst();
}

bool AppendState::continuationResponse(ImapContext *c, const QString &)
{
    AppendParameters &params(mParameters.first());

    QPair<QByteArray, uint> data(params.mData.takeFirst());

    if (params.mData.isEmpty()) {
        // This is the last element
        if (params.mCatenate) {
            data.first.append(")");
        }
        c->sendData(data.first);
        return false;
    } else {
        // There is more literal data to follow
        c->sendDataLiteral(data.first, data.second);
        return true;
    }
}

void AppendState::taggedResponse(ImapContext *c, const QString &line)
{
    if (status() == OpOk) {
        // See if we got an APPENDUID response
        QRegExp appenduidResponsePattern("APPENDUID (\\S+) ([^ \\t\\]]+)");
        appenduidResponsePattern.setCaseSensitivity(Qt::CaseInsensitive);
        if (appenduidResponsePattern.indexIn(line) != -1) {
            const AppendParameters &params(mParameters.first());
            emit messageCreated(params.mMessageId, messageUid(params.mDestination.id(), appenduidResponsePattern.cap(2)));
        }
    }
    
    ImapState::taggedResponse(c, line);
}


// Handles untagged responses in the selected IMAP state
class SelectedState : public ImapState
{
    Q_OBJECT

public:
    SelectedState(ImapCommand c, const QString &name) : ImapState(c, name) {}

    virtual void untaggedResponse(ImapContext *c, const QString &line);
};

void SelectedState::untaggedResponse(ImapContext *c, const QString &line)
{
    bool result;

    if (line.indexOf("EXISTS", 0, Qt::CaseInsensitive) != -1) {
        int start = 0;
        QString temp = token(line, ' ', ' ', &start);
        quint32 exists = temp.toUInt(&result);
        if (!result)
            exists = 0;
        c->setExists(exists);
    } else if (line.indexOf("RECENT", 0, Qt::CaseInsensitive) != -1) {
        int start = 0;
        QString temp = token(line, ' ', ' ', &start);
        quint32 recent = temp.toUInt(&result);
        if (!result)
           recent = 0;
        c->setRecent(recent);
    } else if (line.startsWith("* FLAGS", Qt::CaseInsensitive)) {
        int start = 0;
        QString flags = token(line, '(', ')', &start);
        c->setFlags(flags);
    } else if (line.indexOf("UIDVALIDITY", 0, Qt::CaseInsensitive) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        c->setUidValidity(temp.mid(12).trimmed());
    } else if (line.indexOf("UIDNEXT", 0, Qt::CaseInsensitive) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        QString nextStr = temp.mid( 8 );
        quint32 next = nextStr.toUInt(&result);
        if (!result)
            next = 0;
        c->setUidNext(next);
    } else if (line.indexOf("UNSEEN", 0, Qt::CaseInsensitive) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        QString unseenStr = temp.mid( 7 );
        quint32 unseen = unseenStr.toUInt(&result);
        if (!result)
            unseen = 0;
        c->setUnseen(unseen);
    } else if (line.indexOf("HIGHESTMODSEQ", 0, Qt::CaseInsensitive) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        c->setHighestModSeq(temp.mid(14).trimmed());
    } else if (line.indexOf("NOMODSEQ", 0, Qt::CaseInsensitive) != -1) {
        c->setNoModSeq();
    } else if (line.indexOf("PERMANENTFLAGS", 0, Qt::CaseInsensitive) != -1) {
        int start = 0;
        QString temp = token(line, '(', ')', &start);
        c->setPermanentFlags(temp.split(' ', QString::SkipEmptyParts));
    } else if (line.indexOf("EXPUNGE", 0, Qt::CaseInsensitive) != -1) {
        quint32 exists = c->exists();
        if (exists > 0) {
            --exists;
            c->setExists(exists);
        } else {
            qWarning() << "Unexpected expunge from empty message list";
        }
    } else {
        ImapState::untaggedResponse(c, line);
    }
}


class SelectState : public SelectedState
{
    Q_OBJECT

public:
    SelectState() : SelectedState(IMAP_Select, "Select") { SelectState::init(); }

    void setMailbox(const QMailFolder &mailbox);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void enter(ImapContext *c);
    virtual void leave(ImapContext *);

protected:
    SelectState(ImapCommand c, const QString &name) : SelectedState(c, name) {}

    // The list of mailboxes we're selecting (via multiple commands), in order
    QList<QMailFolder> _mailboxList;
};

void SelectState::setMailbox(const QMailFolder &mailbox)
{
    _mailboxList.append(mailbox);
}

void SelectState::init()
{
    ImapState::init();
    _mailboxList.clear();
}

QString SelectState::transmit(ImapContext *c)
{
    QString cmd("SELECT " + ImapProtocol::quoteString(_mailboxList.last().path()));

    if (c->protocol()->capabilities().contains("CONDSTORE")) {
        cmd.append(" (CONDSTORE)");
    }

    return c->sendCommand(cmd);
}

void SelectState::enter(ImapContext *c)
{
    c->setMailbox(_mailboxList.first());
}

void SelectState::leave(ImapContext *)
{
    ImapState::init();
    _mailboxList.removeFirst();
}


class QResyncState : public SelectState
{
    Q_OBJECT

public:
    QResyncState() : SelectState(IMAP_QResync, "QResync") { init(); }

    virtual QString transmit(ImapContext *c);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
    virtual void taggedResponse(ImapContext *c, const QString &line);
    virtual void enter(ImapContext *c);
    virtual void leave(ImapContext *c);

protected:
    QString vanished;
    QList<FlagChange> changes;
};

void QResyncState::enter(ImapContext *c)
{
    vanished.clear();
    changes.clear();
    SelectState::enter(c);
}

void QResyncState::leave(ImapContext *c)
{
    SelectState::leave(c);
}

QString QResyncState::transmit(ImapContext *c)
{
    QMailFolder folder(_mailboxList.last());
    QString cmd("SELECT " + ImapProtocol::quoteString(folder.path()));
    QString uidValidity(folder.customField("qmf-uidvalidity"));
    QString highestModSeq(folder.customField("qmf-highestmodseq"));
    QString minServerUid(folder.customField("qmf-min-serveruid"));
    QString maxServerUid(folder.customField("qmf-max-serveruid"));
    if (!uidValidity.isEmpty() && !highestModSeq.isEmpty() && !minServerUid.isEmpty() && !maxServerUid.isEmpty()) {
        cmd += QString(" (QRESYNC (%1 %2 %3:%4))").arg(uidValidity).arg(highestModSeq).arg(minServerUid).arg(maxServerUid);
    } else {
        cmd.append(" (CONDSTORE)");
    }
    
    return c->sendCommand(cmd);
}

void QResyncState::untaggedResponse(ImapContext *c, const QString &line)
{
    QString str = line;
    QRegExp fetchResponsePattern("\\*\\s+\\d+\\s+(\\w+)");
    QRegExp vanishResponsePattern("\\*\\s+\\VANISHED\\s+\\(EARLIER\\)\\s+(\\S+)");
    vanishResponsePattern.setCaseSensitivity(Qt::CaseInsensitive);
    if ((fetchResponsePattern.indexIn(str) == 0) && (fetchResponsePattern.cap(1).compare("FETCH", Qt::CaseInsensitive) == 0)) {
        QString uid = extractUid(str, c->mailbox().id);
        if (!uid.isEmpty()) {
            MessageFlags flags = 0;
            parseFlags(str, flags);
            changes.append(FlagChange(uid, flags));
        }
    } else if (vanishResponsePattern.indexIn(str) == 0) {
        vanished = vanishResponsePattern.cap(1);
    } else {
        SelectState::untaggedResponse(c, line);
    }
}

void QResyncState::taggedResponse(ImapContext *c, const QString &line)
{
    c->setVanished(vanished);
    c->setChanges(changes);
    vanished.clear();
    changes.clear();
    SelectState::taggedResponse(c, line);
}


// Flag fetching, 
// doesn't call createMail, instead updates properties.flagChanges
class FetchFlagsState : public SelectedState
{
    Q_OBJECT

public:
    FetchFlagsState() : SelectedState(IMAP_FetchFlags, "FetchFlags") { FetchFlagsState::init(); }

    void setProperties(const QString &range, const QString &prefix);

    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
    virtual void taggedResponse(ImapContext *c, const QString &line);

protected:
    QList<FlagChange> mChanges;
    IntegerRegion mReceivedMessages;
    QString mRange;
    QString mPrefix;
};

void FetchFlagsState::setProperties(const QString &range, const QString &prefix)
{
    mRange = range;
    mPrefix = prefix;
}

void FetchFlagsState::init() 
{ 
    SelectedState::init();
    mChanges.clear();
}

void FetchFlagsState::untaggedResponse(ImapContext *c, const QString &line)
{
    QString str = line;
    QRegExp fetchResponsePattern("\\*\\s+\\d+\\s+(\\w+)");
    if ((fetchResponsePattern.indexIn(str) == 0) && (fetchResponsePattern.cap(1).compare("FETCH", Qt::CaseInsensitive) == 0)) {
        QString uid = extractUid(str, c->mailbox().id);
        if (!uid.isEmpty()) {
            MessageFlags flags = 0;
            parseFlags(str, flags);
            
            bool ok;
            int uidStripped = messageId(uid).toInt(&ok);
            if (!ok)
                return;
            
            mChanges.append(FlagChange(uid, flags));
            mReceivedMessages.add(uidStripped);
        }
    } else {
        SelectedState::untaggedResponse(c, line);
    }
}

void FetchFlagsState::taggedResponse(ImapContext *c, const QString &line)
{
    c->setChanges(mChanges);
    mChanges.clear();
    c->setUidList(mReceivedMessages.toStringList());
    mReceivedMessages.clear();
    SelectedState::taggedResponse(c, line);
}

QString FetchFlagsState::transmit(ImapContext *c)
{
    QString command = QString("FETCH %1 %2").arg(mRange).arg("(FLAGS UID)");
    if (!mPrefix.isEmpty())
        command = mPrefix.simplified() + " " + command;
    return c->sendCommand(command);
}


class ExamineState : public SelectState
{
    Q_OBJECT

public:
    ExamineState() : SelectState(IMAP_Examine, "Examine") { ExamineState::init(); }

    virtual QString transmit(ImapContext *c);
    virtual void enter(ImapContext *c);
};

QString ExamineState::transmit(ImapContext *c)
{
    QString cmd("EXAMINE " + ImapProtocol::quoteString(_mailboxList.last().path()));

    if (c->protocol()->capabilities().contains("CONDSTORE")) {
        cmd.append(" (CONDSTORE)");
    }

    return c->sendCommand(cmd);
}

void ExamineState::enter(ImapContext *c)
{
    // even though we're really in _mailboxList.first() folder, we can't do any (write) operations
    c->setMailbox(QMailFolder());
}

class SearchMessageState : public SelectedState
{
    Q_OBJECT
public:
    SearchMessageState() : SelectedState(IMAP_Search_Message, "Search_Message"), _utf8(false) { }
    virtual bool permitsPipelining() const { return true; }
    void setParameters(const QMailMessageKey &key, const QString &body, const QMailMessageSortKey &sort, bool count);
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual bool continuationResponse(ImapContext *c, const QString &line);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
protected:
    bool isPrintable(const QString &s) const;
    QStringList convertValue(const QVariant &value, const QMailMessageKey::Property &property, const QMailKey::Comparator &comparer);
    QStringList combine(const QList<QStringList> &searchKeys, const QMailKey::Combiner &combiner) const;
    QStringList convertKey(const QMailMessageKey &key);

    struct SearchArgument {
        QMailMessageKey key;
        QString body;
        QMailMessageSortKey sort;
        bool count;
    };
    QList<SearchArgument> _searches;
    QStringList _literals;
    bool _utf8;
    bool _esearch;
};

void SearchMessageState::setParameters(const QMailMessageKey &searchKey, const QString &bodyText, const QMailMessageSortKey &sortKey, bool count)
{
    SearchArgument arg;
    arg.key = searchKey;
    arg.body = bodyText;
    arg.sort = sortKey;
    arg.count = count;
    _searches.append(arg);
    _literals.clear();
    _utf8 = false;
    _esearch = false;
}

QString SearchMessageState::transmit(ImapContext *c)
{
    const SearchArgument &search = _searches.last();
    QStringList searchQueries = convertKey(search.key);

    QString prefix = "UID SEARCH ";
    _utf8 |= !(isPrintable(search.body));
    if (search.count && c->protocol()->capabilities().contains("ESEARCH", Qt::CaseInsensitive)) {
        prefix.append("RETURN (COUNT) ");
        _esearch = true;
    }
    if (_utf8)
        prefix.append("CHARSET UTF-8 ");
    if (!search.body.isEmpty())
        prefix.append("OR (");
    searchQueries.prepend(searchQueries.takeFirst().prepend(prefix));

    if (!search.body.isEmpty()) {
        QString tempLast = searchQueries.takeLast();
        QString searchBody = search.body.toUtf8(); //utf8 is backwards compatible with 7 bit ascii
        searchQueries.append(tempLast + QString(") (BODY {%2}").arg(searchBody.size()));
        searchQueries.append(searchBody + ")");
    }

    QString suffix = " NOT DELETED"; //needed because of limitations in fetching deleted messages
    QString tempLast = searchQueries.takeLast();
    searchQueries.append(tempLast.append(suffix));

    QString first = searchQueries.takeFirst();
    _literals = searchQueries;
    return c->sendCommand(first);
}

bool SearchMessageState::continuationResponse(ImapContext *c, const QString &)
{
    c->sendData(_literals.takeFirst());
    return false;
}

bool SearchMessageState::isPrintable(const QString &s) const
{
    for (int i = 0; i < s.length(); ++i) {
        const char c = s[i].toLatin1();
        if (c < 0x20 || c > 0x7e) {
            return false;
        }
    }
    return true;
}

// Sets _utf8 as side effect
QStringList SearchMessageState::convertValue(const QVariant &value, const QMailMessageKey::Property &property,
                                             const QMailKey::Comparator &comparer)
{

    switch(property) {
    case QMailMessageKey::Id:
        break;
    case QMailMessageKey::Type:
        return QStringList(); // TODO: Why are search keys coming in with "message must equal no type"
    case QMailMessageKey::Sender: {
        _utf8 |= !(isPrintable(value.toString()));
        QString sender = value.toString().toUtf8(); // utf8 is backwards compatible with 7 bit ascii
        if (comparer == QMailKey::Equal || comparer == QMailKey::Includes) {
            QStringList result = QStringList(QString("FROM {%1}").arg(sender.size()));
            result.append(QString("%1").arg(QString(sender)));
            return result;
        } else if(comparer == QMailKey::NotEqual || comparer == QMailKey::Excludes) {
            QStringList result = QStringList(QString("NOT (FROM {%1}").arg(sender.size()));
            result.append(QString("%1)").arg(QString(sender)));
            return result;
        } else {
            qWarning() << "Comparer " << comparer << " is unhandled for sender comparison";
        }
        break;
    }
    case QMailMessageKey::ParentFolderId:
        return QStringList();
    case QMailMessageKey::Recipients: {
        _utf8 |= !(isPrintable(value.toString()));
        QString recipients = value.toString().toUtf8(); // utf8 is backwards compatible with 7 bit ascii
        if(comparer == QMailKey::Equal || comparer == QMailKey::Includes) {
            QStringList result = QStringList(QString("OR (BCC {%1}").arg(recipients.size()));
            result.append(QString("%1) (OR (CC {%2}").arg(recipients).arg(recipients.size()));
            result.append(QString("%1) (TO {%2}").arg(recipients).arg(recipients.size()));
            result.append(QString("%1))").arg(recipients));
            return result;
        } else if(comparer == QMailKey::NotEqual || comparer == QMailKey::Excludes) {
            QStringList result = QStringList(QString("NOT (OR (BCC {%1}").arg(recipients.size()));
            result.append(QString("%1) (OR (CC {%2}").arg(recipients).arg(recipients.size()));
            result.append(QString("%1) (TO {%2}").arg(recipients).arg(recipients.size()));
            result.append(QString("%1)))").arg(recipients));
            return result;
        } else {
            qWarning() << "Comparer " << comparer << " is unhandled for recipients comparison";
        }
        break;
    }
    case QMailMessageKey::Subject: {
        _utf8 |=  !(isPrintable(value.toString()));
        QString subject = value.toString().toUtf8(); //utf8 is backwards compatible with 7 bit ascii
        if(comparer == QMailKey::Equal || comparer == QMailKey::Includes) {
            QStringList result = QStringList(QString("SUBJECT {%1}").arg(subject.size()));
            result.append(QString("%1").arg(QString(subject)));
            return result;
        } else if(comparer == QMailKey::NotEqual || comparer == QMailKey::Excludes) {
            QStringList result = QStringList(QString("NOT (SUBJECT {%1}").arg(subject.size()));
            result.append(QString("%1)").arg(QString(subject)));
            return result;
        } else {
            qWarning() << "Comparer " << comparer << " is unhandled for subject comparison";
        }
        break;
    }
    case QMailMessageKey::TimeStamp:
        break;
    case QMailMessageKey::Status:
        break;
    case QMailMessageKey::Conversation:
        break;
    case QMailMessageKey::ReceptionTimeStamp:
        break;
    case QMailMessageKey::ServerUid:
        break;
    case QMailMessageKey::Size: {
        int size = value.toInt();

        if(comparer == QMailKey::GreaterThan)
            return QStringList(QString("LARGER %1").arg(size));
        else if(comparer == QMailKey::LessThan)
            return QStringList(QString("SMALLER %1").arg(size));
        else if(comparer == QMailKey::GreaterThanEqual)
            return QStringList(QString("LARGER %1").arg(size-1)); // imap has no >= search, so convert it to a >
        else if(comparer == QMailKey::LessThanEqual)
            return QStringList(QString("SMALLER %1").arg(size+1)); // ..same with <=
        else if(comparer == QMailKey::Equal) // ..cause real men know how many bytes they're looking for
            return QStringList(QString("LARGER %1 SMALLER %2").arg(size-1).arg(size+1));
        else
            qWarning() << "Unknown comparer: " << comparer << "for size";
        break;
    }
    case QMailMessageKey::ParentAccountId:
        return QStringList();
        break;
    case QMailMessageKey::AncestorFolderIds:
        return QStringList();
        break;
    case QMailMessageKey::ContentType:
        break;
    case QMailMessageKey::PreviousParentFolderId:
        break;
    case QMailMessageKey::ContentScheme:
        break;
    case QMailMessageKey::ContentIdentifier:
        break;
    case QMailMessageKey::InResponseTo:
        break;
    case QMailMessageKey::ResponseType:
        break;
    case QMailMessageKey::Custom:
        break;
    default:
        qWarning() << "Property " << property << " still not handled for search.";
    }
    return QStringList();
}

QStringList SearchMessageState::convertKey(const QMailMessageKey &key)
{
    QStringList result;
    QMailKey::Combiner combiner = key.combiner();

    QList<QMailMessageKey::ArgumentType> args = key.arguments();

    QList<QStringList> argSearches;
    foreach(QMailMessageKey::ArgumentType arg, args) {
        Q_ASSERT(arg.valueList.count() == 1); // shouldn't have more than 1 element.
        QStringList searchKey(convertValue(arg.valueList[0], arg.property, arg.op));
        if (!searchKey.isEmpty()) {
            argSearches.append(searchKey);
        }
    }
    if (argSearches.size())
        result = combine(argSearches, combiner);



    QList<QStringList> subSearchKeys;
    QList<QMailMessageKey> subkeys = key.subKeys();

    foreach(QMailMessageKey subkey, subkeys) {
        QStringList searchKey(convertKey(subkey));
        if (!searchKey.isEmpty())
            subSearchKeys.append(searchKey);
    }
    if(!subSearchKeys.isEmpty()) {
        result += combine(subSearchKeys, combiner);
    }

    return result;
}

QStringList SearchMessageState::combine(const QList<QStringList> &searchKeys, const QMailKey::Combiner &combiner) const
{
    Q_ASSERT(searchKeys.size() >= 1);
    if (searchKeys.size() == 1) {
        return searchKeys.first();
    } else if(combiner == QMailKey::And) {
        // IMAP uses AND so just add a space and we're good to go!
        QStringList result = searchKeys.first();
        for (int i = 1 ; i < searchKeys.count() ; i++) {
            QStringList cur = searchKeys[i];
            if (cur.count()) {
                cur.first().prepend(' ');
                QString last;
                if (result.count()) {
                    last = result.takeLast();
                }
                result.append(last.append(cur.takeFirst()));
                result.append(cur);
            }
        }
        return result;
    } else if(combiner == QMailKey::Or) {
        QStringList result;

        for (int i = 0 ; i < searchKeys.count() ; i++) {
            QStringList cur = searchKeys[i];
            if (cur.count()) {
                if (i == searchKeys.count() - 1) {// the last one
                    cur[cur.count() - 1].append(QString(')').repeated(searchKeys.count() - 1));
                } else {
                    cur.first().prepend("OR (");
                    cur.last().append(") (");
                }
                QString last;
                if (result.count()) {
                    last = result.takeLast();
                }
                result.append(last.append(cur.takeFirst()));
                result.append(cur);
            } // else should not happen
        }

        return result;
    } else if(combiner == QMailKey::None) {
        if(searchKeys.count() != 1) {
            qWarning() << "Attempting to combine more than thing, without a combiner?";
            return QStringList();
        } else {
            return searchKeys.first();
        }
    } else {
        qWarning() << "Unable to combine with an unknown combiner: " << combiner;
        return QStringList();
    }
}


void SearchMessageState::leave(ImapContext *)
{
    _searches.removeFirst();
}

void SearchMessageState::untaggedResponse(ImapContext *c, const QString &line)
{
    if (line.startsWith(QLatin1String("* ESEARCH"))) {
        int index = 8;
        QString temp;
        QString check;
        QString countStr;
        uint count = 0;
        bool ok;
        while (!(temp = token(line, ' ', ' ', &index)).isEmpty()) {
            check = temp;
            index--;
        }
        countStr = token(line, ' ', '\n', &index);
        if (check.toLower() != "count") {
            qWarning() << "Bad ESEARCH result, count expected";
        }
        count = countStr.toUInt(&ok);
        c->setUidList(QStringList());
        c->setSearchCount(count);
    } else if (line.startsWith(QLatin1String("* SEARCH"))) {
        QStringList uidList;

        int index = 7;
        QString temp;
        while (!(temp = token(line, ' ', ' ', &index)).isEmpty()) {
            uidList.append(messageUid(c->mailbox().id, temp));
            index--;
        }
        temp = token(line, ' ', '\n', &index);
        if (!temp.isEmpty())
            uidList.append(messageUid(c->mailbox().id, temp));
        c->setUidList(uidList);
        c->setSearchCount(uidList.count());
    } else {
        SelectedState::untaggedResponse(c, line);
    }
}

class SearchState : public SelectedState
{
    Q_OBJECT

public:
    SearchState() : SelectedState(IMAP_Search, "Search") { SearchState::init(); }

    void setParameters(MessageFlags flags, const QString &range);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
    virtual QString error(const QString &line);

private:
    // The list of flags/range pairs we're listing (via multiple commands), in order
    QList<QPair<MessageFlags, QString> > _parameters;
};

void SearchState::setParameters(MessageFlags flags, const QString &range)
{
    _parameters.append(qMakePair(flags, range));
}

void SearchState::init()
{
    SelectedState::init();
    _parameters.clear();
}

QString SearchState::transmit(ImapContext *c)
{
    const QPair<MessageFlags, QString> &params(_parameters.last());

    QString flagStr;
    if ((params.first == 0) && params.second.isEmpty()) {
        flagStr = "ALL";
    } else {
        flagStr = searchFlagsToString(params.first);
    }

    if (!params.second.isEmpty() && !flagStr.isEmpty())
        flagStr.prepend(' ');

    return c->sendCommand(QString("SEARCH %1%2").arg(params.second).arg(flagStr));
}

void SearchState::leave(ImapContext *)
{
    SelectedState::init();
    _parameters.removeFirst();
}

void SearchState::untaggedResponse(ImapContext *c, const QString &line)
{
    if (line.startsWith("* SEARCH")) {
        QList<uint> numbers;

        int index = 7;
        QString temp;
        while (!(temp = token(line, ' ', ' ', &index)).isNull()) {
            numbers.append(temp.toUInt());
            index--;
        }
        temp = token(line, ' ', '\n', &index);
        if (!temp.isNull())
            numbers.append(temp.toUInt());
        c->setMsnList(numbers);
    } else {
        SelectedState::untaggedResponse(c, line);
    }
}

QString SearchState::error(const QString &line)
{
    return line + '\n' + QObject::tr( "This server does not provide a complete "
                       "IMAP4rev1 implementation." );
}


class UidSearchState : public SelectedState
{
    Q_OBJECT

public:
    UidSearchState() : SelectedState(IMAP_UIDSearch, "UIDSearch") { UidSearchState::init(); }

    void setParameters(MessageFlags flags, const QString &range);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
    virtual QString error(const QString &line);

private:
    // The list of flags/range pairs we're listing (via multiple commands), in order
    QList<QPair<MessageFlags, QString> > _parameters;
};

void UidSearchState::setParameters(MessageFlags flags, const QString &range)
{
    _parameters.append(qMakePair(flags, range));
}

void UidSearchState::init()
{
    SelectedState::init();
    _parameters.clear();
}

QString UidSearchState::transmit(ImapContext *c)
{
    const QPair<MessageFlags, QString> &params(_parameters.last());

    QString flagStr;
    if ((params.first == 0) && params.second.isEmpty()) {
        flagStr = "ALL";
    } else {
        flagStr = searchFlagsToString(params.first);
    }

    if (!params.second.isEmpty() && !flagStr.isEmpty())
        flagStr.prepend(' ');

    return c->sendCommand(QString("UID SEARCH %1%2").arg(params.second).arg(flagStr));
}

void UidSearchState::leave(ImapContext *)
{
    SelectedState::init();
    _parameters.removeFirst();
}

void UidSearchState::untaggedResponse(ImapContext *c, const QString &line)
{
    if (line.startsWith("* SEARCH")) {
        QStringList uidList;

        int index = 7;
        QString temp;
        while (!(temp = token(line, ' ', ' ', &index)).isNull()) {
            uidList.append(messageUid(c->mailbox().id, temp));
            index--;
        }
        temp = token(line, ' ', '\n', &index);
        if (!temp.isNull())
            uidList.append(messageUid(c->mailbox().id, temp));
        c->setUidList(uidList);
    } else {
        SelectedState::untaggedResponse(c, line);
    }
}

QString UidSearchState::error(const QString &line)
{
    return line + QLatin1String("\n")
        + QObject::tr( "This server does not provide a complete "
                       "IMAP4rev1 implementation." );
}


class UidFetchState : public SelectedState
{
    Q_OBJECT

public:
    UidFetchState() : SelectedState(IMAP_UIDFetch, "UIDFetch") { UidFetchState::init(); }

    void setUidList(const QString &uidList, FetchItemFlags flags);
    void setSection(const QString &uid, const QString &section, int start, int end, FetchItemFlags flags);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
    virtual void taggedResponse(ImapContext *c, const QString &line);
    virtual void literalResponse(ImapContext *c, const QString &line);
    virtual bool appendLiteralData(ImapContext *c, const QString &preceding);
    
signals:
    void downloadSize(const QString&, int);
    void nonexistentUid(const QString&);

private:
    struct FetchParameters
    {
        FetchParameters();

        int mReadLines;
        int mMessageLength;
        QString mNewMsgUid;
        MessageFlags mNewMsgFlags;
        QDateTime mDate;
        uint mNewMsgSize;
        QStringList mNewMsgStructure;
        IntegerRegion mExpectedMessages;
        IntegerRegion mReceivedMessages;
        FetchItemFlags mDataItems;
        QString mUidList;
        QString mSection;
        int mStart;
        int mEnd;
        QString mDetachedFile;
        int mDetachedSize;
    };

    QList<FetchParameters> mParameters;
    int mCurrentIndex;
    QMap<QString, int> mParametersMap;
    int mLiteralIndex;
    
    static QString fetchResponseElement(const QString &line);

    static const int MAX_LINES = 30;
};

UidFetchState::FetchParameters::FetchParameters()
    : mReadLines(0),
      mMessageLength(0),
      mNewMsgFlags(false),
      mNewMsgSize(0),
      mDataItems(0)
{ 
}

void UidFetchState::init() 
{ 
    SelectedState::init();
    mParametersMap.clear();
    mParameters.clear();
    mCurrentIndex = -1;
    mLiteralIndex = -1;
}

void UidFetchState::setUidList(const QString &uidList, FetchItemFlags flags)
{
    int appendIndex = mParameters.count();
    mParameters.append(FetchParameters());

    mParameters.last().mDataItems = flags;
    mParameters.last().mUidList = uidList;
    mParameters.last().mExpectedMessages = IntegerRegion(uidList);

    foreach (int uid, IntegerRegion::toList(uidList)) {
        mParametersMap.insert(QString::number(uid), appendIndex);
    }
    
    if (mCurrentIndex == -1) {
        mCurrentIndex = 0;
    }
}

void UidFetchState::setSection(const QString &uid, const QString &section, int start, int end, FetchItemFlags flags)
{
    int appendIndex = mParameters.count();
    mParameters.append(FetchParameters());

    mParameters.last().mDataItems = flags;
    mParameters.last().mUidList = uid;
    mParameters.last().mSection = section;
    mParameters.last().mStart = start;
    mParameters.last().mEnd = end;

    QString element(uid + ' ' + (section.isEmpty() ? "TEXT" : section));
    if (flags & F_SectionHeader) {
        element.append(".MIME");
    }
    if (end > 0) {
        element.append(QString("<%1>").arg(QString::number(start)));
    }
    mParametersMap.insert(element, appendIndex);

    if (mCurrentIndex == -1) {
        mCurrentIndex = 0;
    }
}

QString UidFetchState::transmit(ImapContext *c)
{
    const FetchParameters &params(mParameters.last());

    QString flagStr;
    if (params.mDataItems & F_Flags)
        flagStr += " FLAGS";
    if (params.mDataItems & F_Uid)
        flagStr += " UID";
    if (params.mDataItems & F_Date)
        flagStr += " INTERNALDATE";
    if (params.mDataItems & F_Rfc822_Size)
        flagStr += " RFC822.SIZE";
    if (params.mDataItems & F_BodyStructure)
        flagStr += " BODYSTRUCTURE";
    if (params.mDataItems & F_Rfc822_Header)
        flagStr += " RFC822.HEADER";
    if (params.mDataItems & F_Rfc822)
        flagStr += " BODY.PEEK[]";
    if (params.mDataItems & F_SectionHeader) {
        flagStr += " BODY.PEEK[";
        if (params.mSection.isEmpty()) {
            flagStr += "HEADER]";
        } else {
            flagStr += params.mSection + ".MIME]";
        }
    }
    if (params.mDataItems & F_BodySection) {
        flagStr += " BODY.PEEK[";
        if (params.mSection.isEmpty()) {
            flagStr += "TEXT]";
        } else {
            flagStr += params.mSection + "]";
        }
        if (params.mEnd > 0) {
            QString strStart = QString::number(params.mStart);
            QString strLen = QString::number(params.mEnd - params.mStart + 1);
            flagStr += ('<' + strStart + '.' + strLen + '>');
        }
    }

    if (!flagStr.isEmpty())
        flagStr = "(" + flagStr.trimmed() + ")";

    return c->sendCommand(QString("UID FETCH %1 %2").arg(params.mUidList).arg(flagStr));
}

void UidFetchState::leave(ImapContext *)
{
    SelectedState::init();
    ++mCurrentIndex;
}

void UidFetchState::untaggedResponse(ImapContext *c, const QString &line)
{
    QString str = line;
    QRegExp fetchResponsePattern("\\*\\s+\\d+\\s+(\\w+)");
    if ((fetchResponsePattern.indexIn(str) == 0) && (fetchResponsePattern.cap(1).compare("FETCH", Qt::CaseInsensitive) == 0)) {
        QString element(fetchResponseElement(str));
        QMap<QString, int>::iterator it = mParametersMap.find(element);
        if (it != mParametersMap.end()) {
            FetchParameters &fp(mParameters[*it]);

            if (fp.mDataItems & F_Uid) {
                fp.mNewMsgUid = extractUid(str, c->mailbox().id);

                bool ok = false;
                int index = fp.mNewMsgUid.lastIndexOf(UID_SEPARATOR);
                if (index == -1)
                    return;
                int uid = fp.mNewMsgUid.mid(index + 1).toInt(&ok);
                if (!ok)
                    return;
                fp.mReceivedMessages.add(uid);
                fp.mMessageLength = 0;
                
                // See what we can extract from the FETCH response
                fp.mNewMsgFlags = 0;
                if (fp.mDataItems & F_Flags) {
                    parseFlags(str, fp.mNewMsgFlags);
                }

                if (fp.mDataItems & F_Date) {
                    fp.mDate = extractDate(str);
                }

                if (fp.mDataItems & F_Rfc822_Size) {
                    fp.mNewMsgSize = extractSize(str);
                }

                if (!c->literalResponseCompleted()) {
                    // Wait for the literal data to be received
                    mLiteralIndex = *it;
                    return;
                }

                // All message data should be retrieved at this point - create new mail/part
                if (fp.mDataItems & F_BodyStructure) {
                    fp.mNewMsgStructure = extractStructure(str);
                }

                if ((fp.mDataItems & F_BodySection)
                    || (fp.mDataItems & F_SectionHeader)) {
                    if (fp.mDetachedFile.isEmpty()) {
                        // The buffer has not been detached to a file yet
                        fp.mDetachedSize = c->buffer().length();
                        fp.mDetachedFile = c->buffer().detach();
                    }
                    if (fp.mDataItems & F_SectionHeader) {
                        c->createPartHeader(fp.mNewMsgUid, fp.mSection, fp.mDetachedFile, fp.mDetachedSize);
                    } else {
                        c->createPart(fp.mNewMsgUid, fp.mSection, fp.mDetachedFile, fp.mDetachedSize);
                    }
                } else {
                    if (fp.mNewMsgSize == 0) {
                        fp.mNewMsgSize = fp.mDetachedSize;
                    }
                    c->createMail(fp.mNewMsgUid, fp.mDate, fp.mNewMsgSize, fp.mNewMsgFlags, fp.mDetachedFile, fp.mNewMsgStructure);
                }
            }
        } else {
            qWarning() << "untaggedResponse: Unable to find fetch parameters for:" << str;
        }
    } else {
        SelectedState::untaggedResponse(c, line);
    }
}

void UidFetchState::taggedResponse(ImapContext *c, const QString &line)
{
    if (status() == OpOk) {
        FetchParameters &fp(mParameters[mCurrentIndex]);

        IntegerRegion missingUids = fp.mExpectedMessages.subtract(fp.mReceivedMessages);
        foreach(const QString &uid, missingUids.toStringList()) {
            qWarning() << "Message not found " << uid;
            emit nonexistentUid(messageUid(c->mailbox().id, uid));
        } 
    } 

    SelectedState::taggedResponse(c, line);
}

void UidFetchState::literalResponse(ImapContext *c, const QString &line)
{
    if (!c->literalResponseCompleted()) {
        if (mLiteralIndex != -1) {
            FetchParameters &fp(mParameters[mLiteralIndex]);

            ++fp.mReadLines;
            if ((fp.mDataItems & F_Rfc822) || (fp.mDataItems & F_BodySection)) {
                fp.mMessageLength += line.length();

                if (fp.mReadLines > MAX_LINES) {
                    fp.mReadLines = 0;
                    emit downloadSize(fp.mNewMsgUid, fp.mMessageLength);
                }
            }
        } else {
            qWarning() << "Literal data received with invalid literal index!";
        }
    }
}

bool UidFetchState::appendLiteralData(ImapContext *c, const QString &preceding)
{
    if (mLiteralIndex != -1) {
        FetchParameters &fp(mParameters[mLiteralIndex]);

        // We're finished with this literal data
        mLiteralIndex = -1;

        QRegExp pattern;
        if (fp.mDataItems & F_Rfc822_Header) {
            // If the literal is the header data, keep it in the buffer file
            pattern = QRegExp("RFC822\\.HEADER ");
        } else {
            // If the literal is the body data, keep it in the buffer file
            pattern = QRegExp("BODY\\[\\S*\\] ");
        }

        pattern.setCaseSensitivity(Qt::CaseInsensitive);
        int index = pattern.lastIndexIn(preceding);
        if (index != -1) {
            if ((index + pattern.cap(0).length()) == preceding.length()) {
                // Detach the retrieved data to a file we have ownership of
                fp.mDetachedSize = c->buffer().length();
                fp.mDetachedFile = c->buffer().detach();
                return false;
            }
        }
    } else {
        qWarning() << "Literal data appended with invalid literal index!";
    }

    return true;
}

QString UidFetchState::fetchResponseElement(const QString &line)
{
    QString result;

    QRegExp uidPattern("UID\\s+(\\d+)");
    uidPattern.setCaseSensitivity(Qt::CaseInsensitive);
    if (uidPattern.indexIn(line) != -1) {
        result = uidPattern.cap(1);
    }

    QRegExp bodyPattern("BODY\\[([^\\]]*)\\](<[^>]*>)?");
    bodyPattern.setCaseSensitivity(Qt::CaseInsensitive);
    if (bodyPattern.indexIn(line) != -1) {
        QString section(bodyPattern.cap(1));
        if (!section.isEmpty()) {
            result.append(' ' + section + bodyPattern.cap(2));
        }
    }

    return result;
}


class UidStoreState : public SelectedState
{
    Q_OBJECT

public:
    UidStoreState() : SelectedState(IMAP_UIDStore, "UIDStore") { UidStoreState::init(); }

    void setParameters(MessageFlags flags, bool set, const QString &range);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *);
    virtual void taggedResponse(ImapContext *c, const QString &line);

signals:
    void messageStored(const QString &uid);

private:
    // The list of flags/range pairs we're storing (via multiple commands), in order
    QList<QPair<QPair<MessageFlags, bool>, QString> > _parameters;
};

void UidStoreState::setParameters(MessageFlags flags, bool set, const QString &range)
{
    _parameters.append(qMakePair(qMakePair(flags, set), range));
}

void UidStoreState::init()
{
    SelectedState::init();
    _parameters.clear();
}

QString UidStoreState::transmit(ImapContext *c)
{
    const QPair<QPair<MessageFlags, bool>, QString> &params(_parameters.last());

    QString flagStr = QString("FLAGS.SILENT (%1)").arg(messageFlagsToString(params.first.first));
    return c->sendCommand(QString("UID STORE %1 %2%3").arg(params.second).arg(params.first.second ? '+' : '-').arg(flagStr));
}

void UidStoreState::leave(ImapContext *)
{
    SelectedState::init();
    _parameters.removeFirst();
}

void UidStoreState::taggedResponse(ImapContext *c, const QString &line)
{
    if (status() == OpOk) {
        const QPair<QPair<MessageFlags, bool>, QString> &params(_parameters.first());

        // Report all UIDs stored
        foreach (uint uid, sequenceUids(params.second))
            emit messageStored(messageUid(c->mailbox().id, QString::number(uid)));
    }

    SelectedState::taggedResponse(c, line);
}


class UidCopyState : public SelectedState
{
    Q_OBJECT

public:
    UidCopyState() : SelectedState(IMAP_UIDCopy, "UIDCopy") { UidCopyState::init(); }

    void setParameters(const QString &range, const QMailFolder &destination);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *);
    virtual void taggedResponse(ImapContext *c, const QString &line);

signals:
    void messageCopied(const QString&, const QString&);

private:
    // The list of range/mailbox pairs we're copying (via multiple commands), in order
    QList<QPair<QString, QMailFolder> > _parameters;
};

void UidCopyState::setParameters(const QString &range, const QMailFolder &destination)
{
    _parameters.append(qMakePair(range, destination));
}

void UidCopyState::init()
{
    SelectedState::init();
    _parameters.clear();
}

QString UidCopyState::transmit(ImapContext *c)
{
    const QPair<QString, QMailFolder> &params(_parameters.last());

    return c->sendCommand(QString("UID COPY %1 %2").arg(params.first).arg(ImapProtocol::quoteString(params.second.path())));
}

void UidCopyState::leave(ImapContext *)
{
    SelectedState::init();
    _parameters.removeFirst();
}

void UidCopyState::taggedResponse(ImapContext *c, const QString &line)
{
    if (status() == OpOk) {
        const QPair<QString, QMailFolder> &params(_parameters.first());

        // See if we got a COPYUID response
        QRegExp copyuidResponsePattern("COPYUID (\\S+) (\\S+) ([^ \\t\\]]+)");
        copyuidResponsePattern.setCaseSensitivity(Qt::CaseInsensitive);
        if (copyuidResponsePattern.indexIn(line) != -1) {
            QList<uint> copiedUids = sequenceUids(copyuidResponsePattern.cap(2));
            QList<uint> createdUids = sequenceUids(copyuidResponsePattern.cap(3));

            // Report the completed copies
            if (copiedUids.count() != createdUids.count()) {
                qWarning() << "Mismatched COPYUID output:" << copiedUids << "!=" << createdUids;
            } else {
                const QString &mailbox();

                while (!copiedUids.isEmpty()) {
                    QString copiedUid(messageUid(c->mailbox().id, QString::number(copiedUids.takeFirst())));
                    QString createdUid(messageUid(params.second.id(), QString::number(createdUids.takeFirst())));

                    emit messageCopied(copiedUid, createdUid);
                }
            }
        } else {
            // Otherwise, report all UIDs copied, without the created UIDs
            foreach (uint uid, sequenceUids(params.first)) {
                emit messageCopied(messageUid(c->mailbox().id, QString::number(uid)), QString());
            }
        }
    }
    
    SelectedState::taggedResponse(c, line);
}


class ExpungeState : public SelectedState
{
    Q_OBJECT

public:
    ExpungeState() : SelectedState(IMAP_Expunge, "Expunge") {}

    virtual bool permitsPipelining() const { return true; }
    virtual QString transmit(ImapContext *c);
};

QString ExpungeState::transmit(ImapContext *c)
{
    return c->sendCommand("EXPUNGE");
}


class CloseState : public SelectedState
{
    Q_OBJECT

public:
    CloseState() : SelectedState(IMAP_Close, "Close") {}

    virtual bool permitsPipelining() const { return true; }
    virtual QString transmit(ImapContext *c);
    virtual void taggedResponse(ImapContext *c, const QString &line);
};

QString CloseState::transmit(ImapContext *c)
{
    return c->sendCommand("CLOSE");
}

void CloseState::taggedResponse(ImapContext *c, const QString &line)
{
    if (status() == OpOk) {
        // After a close, we no longer have a selected mailbox
        c->setMailbox(QMailFolder());
    }

    ImapState::taggedResponse(c, line);
}

class EnableState : public ImapState
{
    Q_OBJECT

public:
    EnableState() : ImapState(IMAP_Enable, "Enable") {}

    void setExtensions(const QString &extensions);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual void taggedResponse(ImapContext *c, const QString &line);

private:
    QStringList _extensionsList;
};

void EnableState::setExtensions(const QString &extensions)
{
    _extensionsList.append(extensions);
}

void EnableState::init()
{
    ImapState::init();
    _extensionsList.clear();
}

QString EnableState::transmit(ImapContext *c)
{
    QString cmd("ENABLE " + _extensionsList.last());
    return c->sendCommand(cmd);
}

void EnableState::leave(ImapContext *)
{
    ImapState::init();
    _extensionsList.removeFirst();
}

void EnableState::taggedResponse(ImapContext *c, const QString &line)
{
    ImapState::taggedResponse(c, line);
}

class NoopState : public SelectedState
{
    Q_OBJECT

public:
    NoopState() : SelectedState(IMAP_Noop, "Noop") {}

    virtual bool permitsPipelining() const { return true; }
    virtual QString transmit(ImapContext *c);
};

QString NoopState::transmit(ImapContext *c)
{
    QString cmd("NOOP");
    return c->sendCommand(cmd);
}

class FullState : public ImapState
{
    Q_OBJECT

public:
    FullState() : ImapState(IMAP_Full, "Full") { setStatus(OpFailed); }
};


class IdleState : public SelectedState
{
    Q_OBJECT

public:
    IdleState() : SelectedState(IMAP_Idle, "Idle") {}

    void done(ImapContext *c);

    virtual QString transmit(ImapContext *c);
    virtual bool continuationResponse(ImapContext *c, const QString &line);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
};

void IdleState::done(ImapContext *c)
{
    c->sendData("DONE");
}

QString IdleState::transmit(ImapContext *c)
{
    return c->sendCommand("IDLE");
}

bool IdleState::continuationResponse(ImapContext *c, const QString &)
{
    c->continuation(command(), QString("idling"));
    return false;
}

void IdleState::untaggedResponse(ImapContext *c, const QString &line)
{
    QString str = line;
    QRegExp idleResponsePattern("\\*\\s+\\d+\\s+(\\w+)");
    quint32 previousExists = c->exists();
    SelectedState::untaggedResponse(c, line);
    if (idleResponsePattern.indexIn(str) == 0) {
        // Treat this event as a continuation point
        if (previousExists < c->exists()) { // '<' to avoid double check for expunges
             c->continuation(command(), QString("newmail"));
        } else if (idleResponsePattern.cap(1).compare("FETCH", Qt::CaseInsensitive) == 0) {
            c->continuation(command(), QString("flagschanged"));
        } else if (idleResponsePattern.cap(1).compare("EXPUNGE", Qt::CaseInsensitive) == 0) {
            c->continuation(command(), QString("flagschanged")); // flags check will find expunged messages
        }
    }
}

class CompressState : public ImapState
{
    Q_OBJECT

public:
    CompressState() : ImapState(IMAP_Compress, "Compress") {}

    virtual QString transmit(ImapContext *c);
    virtual void taggedResponse(ImapContext *c, const QString &line);
    virtual bool permitsPipelining() const { return true; }
};

QString CompressState::transmit(ImapContext *c)
{
    return c->sendCommand("COMPRESS DEFLATE");
}

void CompressState::taggedResponse(ImapContext *c, const QString &line)
{
    Q_UNUSED(line);
    c->protocol()->setCompress(true);
    ImapState::taggedResponse(c, line);
}



class ImapContextFSM : public ImapContext
{
public:
    ImapContextFSM(ImapProtocol *protocol); 

    UnconnectedState unconnectedState;
    InitState initState;
    CapabilityState capabilityState;
    StartTlsState startTlsState;
    LoginState loginState;
    LogoutState logoutState;
    ListState listState;
    GenUrlAuthState genUrlAuthState;
    AppendState appendState;
    SelectState selectState;
    QResyncState qresyncState;
    FetchFlagsState uidFetchFlagsState;
    ExamineState examineState;
    CreateState createState;
    DeleteState deleteState;
    EnableState enableState;
    NoopState noopState;
    RenameState renameState;
    MoveState moveState;
    SearchMessageState searchMessageState;
    SearchState searchState;
    UidSearchState uidSearchState;
    UidFetchState uidFetchState;
    UidStoreState uidStoreState;
    UidCopyState uidCopyState;
    ExpungeState expungeState;
    CloseState closeState;
    FullState fullState;
    IdleState idleState;
    CompressState compressState;

    virtual QString sendCommandLiteral(const QString &cmd, uint length);

    bool continuationResponse(const QString &line) { return state()->continuationResponse(this, line); }
    void untaggedResponse(const QString &line) { state()->untaggedResponse(this, line); }
    void taggedResponse(const QString &line) { state()->taggedResponse(this, line); }
    void literalResponse(const QString &line) { state()->literalResponse(this, line); }
    bool appendLiteralData(const QString &preceding) { return state()->appendLiteralData(this, preceding); }

    QString error(const QString &line) const { return state()->error(line); }

    void log(const QString &note) const { state()->log(note); }
    QString tag() const { return state()->tag(); }
    ImapCommand command() const { return state()->command(); }
    OperationStatus status() const { return state()->status(); }

    void setStatus(OperationStatus status) const { return state()->setStatus(status); }

    ImapState* state() const { return mState; }
    void reset();
    void setState(ImapState* s);
    void stateCompleted();

private:
    ImapState* mState;
    QList<QPair<ImapState*, QString> > mPendingStates;
};

ImapContextFSM::ImapContextFSM(ImapProtocol *protocol)
    : ImapContext(protocol),
      mState(&unconnectedState)
{ 
    reset();
}

QString ImapContextFSM::sendCommandLiteral(const QString &cmd, uint length)
{ 
    QString tag(ImapContext::sendCommandLiteral(cmd, length));

    if (protocol()->capabilities().contains("LITERAL+")) {
        // Send the continuation immediately
        while (continuationResponse(QString())) {
            // Keep sending continuations until there are no more
        }
    }

    return tag;
}

void ImapContextFSM::reset()
{
    // Clear any existing state we have accumulated
    while (!mPendingStates.isEmpty()) {
        QPair<ImapState*, QString> state(mPendingStates.takeFirst());
        state.first->init();
    }

    mState->init();
    mState = &unconnectedState;  
}

void ImapContextFSM::setState(ImapState* s)
{
    if (!mPendingStates.isEmpty() || (mState->status() == OpPending)) {
        // This state is not yet active, but its command will be pipelined
        if (!s->permitsPipelining()) {
            qMailLog(IMAP) << protocol()->objectName() << "Unable to issue command simultaneously:" << s->command();
            operationCompleted(s->command(), OpFailed);
        } else {
            s->log(protocol()->objectName() + "Tx:");
            QString tag = s->transmit(this);
            mPendingStates.append(qMakePair(s, tag));
        }
    } else {
        mState->leave(this);
        mState = s;

        // We can enter the new state and transmit its command
        mState->log(protocol()->objectName() + "Begin:");
        QString tag = mState->transmit(this);
        mState->enter(this);
        mState->setTag(tag);
    }
}

void ImapContextFSM::stateCompleted()
{
    if (!mPendingStates.isEmpty() && (mState->status() != OpPending)) {
        // Advance to the next state
        QPair<ImapState *, QString> nextState(mPendingStates.takeFirst());

        // See if we should close the existing state
        mState->leave(this);
        mState = nextState.first;

        // We can now enter the new state
        if (nextState.second.isEmpty()) {
            // This state has not transmitted a command yet
            mState->log(protocol()->objectName() + "Tx:");
            nextState.second = mState->transmit(this);
        }

        mState->log(protocol()->objectName() + "Begin:");
        mState->enter(this);
        mState->setTag(nextState.second);
    }
}


/* End state design pattern classes */

ImapProtocol::ImapProtocol()
    : _fsm(new ImapContextFSM(this)),
      _transport(0),
      _literalDataRemaining(0),
      _flatHierarchy(false),
      _delimiter(0),
      _authenticated(false),
      _receivedCapabilities(false)
{
    connect(&_incomingDataTimer, SIGNAL(timeout()), this, SLOT(incomingData()));
    connect(&_fsm->listState, SIGNAL(mailboxListed(QString, QString)),
            this, SIGNAL(mailboxListed(QString, QString)));
    connect(&_fsm->genUrlAuthState, SIGNAL(urlAuthorized(QString)),
            this, SIGNAL(urlAuthorized(QString)));
    connect(&_fsm->appendState, SIGNAL(messageCreated(QMailMessageId, QString)),
            this, SIGNAL(messageCreated(QMailMessageId, QString)));
    connect(&_fsm->uidFetchState, SIGNAL(downloadSize(QString, int)), 
            this, SIGNAL(downloadSize(QString, int)));
    connect(&_fsm->uidFetchState, SIGNAL(nonexistentUid(QString)), 
            this, SIGNAL(nonexistentUid(QString)));
    connect(&_fsm->uidStoreState, SIGNAL(messageStored(QString)), 
            this, SIGNAL(messageStored(QString)));
    connect(&_fsm->uidCopyState, SIGNAL(messageCopied(QString, QString)), 
            this, SIGNAL(messageCopied(QString, QString)));
    connect(&_fsm->createState, SIGNAL(folderCreated(QString, bool)),
            this, SIGNAL(folderCreated(QString, bool)));
    connect(&_fsm->deleteState, SIGNAL(folderDeleted(QMailFolder, bool)),
            this, SIGNAL(folderDeleted(QMailFolder, bool)));
    connect(&_fsm->renameState, SIGNAL(folderRenamed(QMailFolder, QString, bool)),
            this, SIGNAL(folderRenamed(QMailFolder, QString, bool)));
    connect(&_fsm->moveState, SIGNAL(folderMoved(QMailFolder, QString, QMailFolderId, bool)),
            this, SIGNAL(folderMoved(QMailFolder, QString, QMailFolderId, bool)));
}

ImapProtocol::~ImapProtocol()
{
    delete _transport;
    delete _fsm;
}

bool ImapProtocol::open( const ImapConfiguration& config, qint64 bufferSize)
{
    if ( _transport && _transport->inUse() ) {
        QString msg("Cannot open account; transport in use");
        qMailLog(IMAP) << objectName() << msg;
        emit connectionError(QMailServiceAction::Status::ErrConnectionInUse, msg);
        return false;
    }

    _fsm->reset(); // Recover from previously severed connection
    _fsm->setState(&_fsm->initState);

    _errorList.clear();

    _requestCount = 0;
    _stream.reset();
    _literalDataRemaining = 0;
    _precedingLiteral.clear();

    _mailbox = ImapMailboxProperties();
    _authenticated = false;
    _receivedCapabilities = false;

    if (!_transport) {
        _transport = new ImapTransport("IMAP");

        connect(_transport, SIGNAL(updateStatus(QString)),
                this, SIGNAL(updateStatus(QString)));
        connect(_transport, SIGNAL(errorOccurred(int,QString)),
                this, SLOT(errorHandling(int,QString)));
        connect(_transport, SIGNAL(connected(QMailTransport::EncryptType)),
                this, SLOT(connected(QMailTransport::EncryptType)));
        connect(_transport, SIGNAL(readyRead()),
                this, SLOT(incomingData()));
    }

    qMailLog(IMAP) << objectName() << "About to open connection" << config.mailUserName() << config.mailServer(); // useful to see object name
    _transport->open( config.mailServer(), config.mailPort(), static_cast<QMailTransport::EncryptType>(config.mailEncryption()));
    if (bufferSize) {
        qMailLog(IMAP) << objectName() << "Setting read buffer size to" << bufferSize;
        _transport->socket().setReadBufferSize(bufferSize);
    }

    return true;
}

void ImapProtocol::close()
{
    if (_transport)
        _transport->imapClose();
    _stream.reset();
    _fsm->reset();

    _mailbox = ImapMailboxProperties();
    _authenticated = false;
    _receivedCapabilities = false;
}

bool ImapProtocol::connected() const
{
    return (_transport && _transport->connected());
}

bool ImapProtocol::encrypted() const
{
    return (_transport && _transport->isEncrypted());
}

bool ImapProtocol::inUse() const
{
    return (_transport && _transport->inUse());
}

bool ImapProtocol::delimiterUnknown() const
{
    return !flatHierarchy() && delimiter().isNull();
}

bool ImapProtocol::flatHierarchy() const
{
    return _flatHierarchy;
}

void ImapProtocol::setFlatHierarchy(bool flat)
{
    _flatHierarchy = flat;
}

QChar ImapProtocol::delimiter() const
{
    return _delimiter;
}

void ImapProtocol::setDelimiter(QChar delimiter)
{
    _delimiter = delimiter;
}

bool ImapProtocol::compress() const
{
    return _transport->compress();
}

void ImapProtocol::setCompress(bool comp)
{
    _transport->setCompress(comp);
}

bool ImapProtocol::authenticated() const
{
    return _authenticated;
}

void ImapProtocol::setAuthenticated(bool auth)
{
    _authenticated = auth;
}

// capabilities info has been sent by server, possibly as a result of unsolicited response.
bool ImapProtocol::receivedCapabilities() const
{
    return _receivedCapabilities;
}

void ImapProtocol::setReceivedCapabilities(bool received)
{
    _receivedCapabilities = received;
}

bool ImapProtocol::loggingOut() const
{
    return _fsm->state() == &_fsm->logoutState;
}

const QStringList &ImapProtocol::capabilities() const
{
    return _capabilities;
}

void ImapProtocol::setCapabilities(const QStringList &newCapabilities)
{
    setReceivedCapabilities(true);
    _capabilities = newCapabilities;
}

bool ImapProtocol::supportsCapability(const QString& name) const
{
    return _capabilities.contains(name);
}

void ImapProtocol::sendCapability()
{
    _fsm->setState(&_fsm->capabilityState);
}

void ImapProtocol::sendStartTLS()
{
    _fsm->setState(&_fsm->startTlsState);
}

void ImapProtocol::sendLogin( const QMailAccountConfiguration &config )
{
    _fsm->loginState.setConfiguration(config, _capabilities);
    _fsm->setState(&_fsm->loginState);
}

void ImapProtocol::sendLogout()
{
    _fsm->setState(&_fsm->logoutState);
}

void ImapProtocol::sendNoop()
{
    _fsm->setState(&_fsm->noopState);
}

void ImapProtocol::sendIdle()
{
    _fsm->setState(&_fsm->idleState);
}

void ImapProtocol::sendIdleDone()
{
    if (_fsm->state() == &_fsm->idleState)
        _fsm->idleState.done(_fsm);
}

void ImapProtocol::sendCompress()
{
    _fsm->setState(&_fsm->compressState);
}

void ImapProtocol::sendList( const QMailFolder &reference, const QString &mailbox )
{
    QString path;
    if (!reference.path().isEmpty()) {
        path = reference.path();
    }

    if (!path.isEmpty()) {
        // If we don't have the delimiter for this account, we need to discover it
        if (delimiterUnknown()) {
            sendDiscoverDelimiter();
        }
    }


    // Now request the actual list
    _fsm->listState.setParameters(path, mailbox, capabilities().contains("XLIST"));
    _fsm->setState(&_fsm->listState);
}

void ImapProtocol::sendDiscoverDelimiter()
{
    _fsm->listState.setDiscoverDelimiter();
    _fsm->setState(&_fsm->listState);
}

void ImapProtocol::sendGenUrlAuth(const QMailMessagePart::Location &location, bool bodyOnly, const QString &mechanism)
{
    QString dataUrl(url(location, true, bodyOnly));

    _fsm->genUrlAuthState.setUrl(dataUrl, mechanism);
    _fsm->setState(&_fsm->genUrlAuthState);
}

void ImapProtocol::sendAppend(const QMailFolder &mailbox, const QMailMessageId &messageId)
{
    _fsm->appendState.setParameters(mailbox, messageId);
    _fsm->setState(&_fsm->appendState);
}

void ImapProtocol::sendSelect(const QMailFolder &mailbox)
{
    _fsm->selectState.setMailbox(mailbox);
    _fsm->setState(&_fsm->selectState);
}

void ImapProtocol::sendQResync(const QMailFolder &mailbox)
{
    _fsm->qresyncState.setMailbox(mailbox);
    _fsm->setState(&_fsm->qresyncState);
}

void ImapProtocol::sendExamine(const QMailFolder &mailbox)
{
    _fsm->examineState.setMailbox(mailbox);
    _fsm->setState(&_fsm->examineState);
}

void ImapProtocol::sendCreate(const QMailFolderId &parentFolderId, const QString &name)
{
    QString mailboxPath;
    if(parentFolderId.isValid())
    {
        if (delimiterUnknown()) {
            sendDiscoverDelimiter();
        }
    }
    _fsm->createState.setMailbox(parentFolderId, name);
    _fsm->setState(&_fsm->createState);
}

void ImapProtocol::sendDelete(const QMailFolder &mailbox)
{
    _fsm->deleteState.setMailbox(mailbox);
    _fsm->setState(&_fsm->deleteState);
}

void ImapProtocol::sendRename(const QMailFolder &mailbox, const QString &newName)
{
    if (delimiterUnknown()) {
        sendDiscoverDelimiter();
    }
    _fsm->renameState.setNewMailboxName(mailbox, newName);
    _fsm->setState(&_fsm->renameState);
}

void ImapProtocol::sendMove(const QMailFolder &mailbox, const QMailFolderId &newParentId)
{
    if (delimiterUnknown()) {
        sendDiscoverDelimiter();
    }
    _fsm->moveState.setNewMailboxParent(mailbox, newParentId);
    _fsm->setState(&_fsm->moveState);
}

void ImapProtocol::sendSearchMessages(const QMailMessageKey &key, const QString &body, const QMailMessageSortKey &sort, bool count)
{
    _fsm->searchMessageState.setParameters(key, body, sort, count);
    _fsm->setState(&_fsm->searchMessageState);
}


void ImapProtocol::sendSearch(MessageFlags flags, const QString &range)
{
    _fsm->searchState.setParameters(flags, range);
    _fsm->setState(&_fsm->searchState);
}

void ImapProtocol::sendUidSearch(MessageFlags flags, const QString &range)
{
    _fsm->uidSearchState.setParameters(flags, range);
    _fsm->setState(&_fsm->uidSearchState);
}

void ImapProtocol::sendFetchFlags(const QString &range, const QString &prefix)
{
    _fsm->uidFetchFlagsState.setProperties(range, prefix);
    _fsm->setState(&_fsm->uidFetchFlagsState);
}

void ImapProtocol::sendUidFetch(FetchItemFlags items, const QString &uidList)
{
    _fsm->uidFetchState.setUidList(uidList, (items | F_Flags));
    _fsm->setState(&_fsm->uidFetchState);
}

void ImapProtocol::sendUidFetchSection(const QString &uid, const QString &section, int start, int end)
{
    _fsm->uidFetchState.setSection(uid, section, start, end, (F_Uid | F_BodySection));
    _fsm->setState(&_fsm->uidFetchState);
}

void ImapProtocol::sendUidFetchSectionHeader(const QString &uid, const QString &section)
{
    _fsm->uidFetchState.setSection(uid, section, 0, -1, (F_Uid | F_SectionHeader));
    _fsm->setState(&_fsm->uidFetchState);
}

void ImapProtocol::sendUidStore(MessageFlags flags, bool set, const QString &range)
{
    _fsm->uidStoreState.setParameters(flags, set, range);
    _fsm->setState(&_fsm->uidStoreState);
}

void ImapProtocol::sendUidCopy(const QString &range, const QMailFolder &destination)
{
    _fsm->uidCopyState.setParameters(range, destination);
    _fsm->setState(&_fsm->uidCopyState);
}

void ImapProtocol::sendExpunge()
{
    _fsm->setState(&_fsm->expungeState);
}

void ImapProtocol::sendClose()
{
    _fsm->setState(&_fsm->closeState);
}

void ImapProtocol::sendEnable(const QString &extensions)
{
    _fsm->enableState.setExtensions(extensions);
    _fsm->setState(&_fsm->enableState);
}

void ImapProtocol::connected(QMailTransport::EncryptType encryptType)
{
#ifndef QT_NO_SSL
    if (encryptType == QMailTransport::Encrypt_TLS) {
        emit completed(IMAP_StartTLS, OpOk);
    }
#else
    Q_UNUSED(encryptType);
#endif
}

void ImapProtocol::errorHandling(int status, QString msg)
{
    _mailbox = ImapMailboxProperties();

    if (msg.isEmpty())
        msg = tr("Connection failed");

    if (_fsm->command() != IMAP_Logout)
        emit connectionError(status, msg);
}

void ImapProtocol::sendData(const QString &cmd, bool maskDebug)
{
    QByteArray output(cmd.toLatin1());
    output.append("\r\n");
    _transport->imapWrite(&output);

    if (maskDebug) {
        qMailLog(IMAP) << objectName() << (compress() ? "SENDC:" : "SEND") << "SEND: <login hidden>";
    } else {
        QString logCmd(cmd);
        QRegExp authExp("^[^\\s]+\\sAUTHENTICATE\\s[^\\s]+\\s");
        if (authExp.indexIn(cmd) != -1) {
            logCmd = cmd.left(authExp.matchedLength()) + "<password hidden>";
        } else {
            QRegExp loginExp("^[^\\s]+\\sLOGIN\\s[^\\s]+\\s");
            if (loginExp.indexIn(cmd) != -1) {
                logCmd = cmd.left(loginExp.matchedLength()) + "<password hidden>";
            }
        }
        qMailLog(IMAP) << objectName() << (compress() ? "SENDC:" : "SEND") << qPrintable(logCmd);}
}

void ImapProtocol::sendDataLiteral(const QString &cmd, uint length)
{
    QString trailer(" {%1%2}");
    trailer = trailer.arg(length);
    trailer = trailer.arg(capabilities().contains("LITERAL+") ? "+" : "");

    sendData(cmd + trailer);
}

QString ImapProtocol::sendCommand(const QString &cmd)
{
    QString tag = newCommandId();

    _stream.reset();
    sendData(tag + ' ' + cmd);

    return tag;
}

QString ImapProtocol::sendCommandLiteral(const QString &cmd, uint length)
{
    QString trailer(" {%1%2}");
    trailer = trailer.arg(length);
    trailer = trailer.arg(capabilities().contains("LITERAL+") ? "+" : "");

    return sendCommand(cmd + trailer);
}

void ImapProtocol::incomingData()
{
    if (!_lineBuffer.isEmpty() && _transport->imapCanReadLine()) {
        processResponse(QString::fromLatin1(_lineBuffer + _transport->imapReadLine()));
        _lineBuffer.clear();
    }

    int readLines = 0;
    while (_transport->imapCanReadLine()) {
        processResponse(QString::fromLatin1(_transport->imapReadLine()));

        readLines++;
        if (readLines >= MAX_LINES) {
            _incomingDataTimer.start(0);
            return;
        }
    }

    if (_transport->bytesAvailable()) {
        // If there is an incomplete line, read it from the socket buffer to ensure we get readyRead signal next time
        _lineBuffer.append(_transport->readAll());
    }

    _incomingDataTimer.stop();
}

void ImapProtocol::continuation(ImapCommand command, const QString &recv)
{
    clearResponse();

    emit continuationRequired(command, recv);
}

void ImapProtocol::operationCompleted(ImapCommand command, OperationStatus status)
{
    _fsm->state()->log(objectName() + "End:");
    clearResponse();

    emit completed(command, status);
}

void ImapProtocol::clearResponse()
{
    _stream.reset();
}

int ImapProtocol::literalDataRemaining() const
{
    return _literalDataRemaining;
}

void ImapProtocol::setLiteralDataRemaining(int literalDataRemaining)
{
    _literalDataRemaining = literalDataRemaining;
}

QString ImapProtocol::precedingLiteral() const
{
    return _precedingLiteral;
}

void ImapProtocol::setPrecedingLiteral(const QString &line)
{
    _precedingLiteral = line;
}

bool ImapProtocol::checkSpace()
{
    if (_stream.status() == LongStream::OutOfSpace) {
        _lastError += LongStream::errorMessage( QString('\n') );
        clearResponse();
        return false;
    }

    return true;
}

void ImapProtocol::processResponse(QString line)
{
    int outstandingLiteralData = literalDataRemaining();
    if (outstandingLiteralData > 0) {
        // This should be a literal data response
        QString literal, remainder;

        // See if there is any line data trailing the embedded literal data
        int excess = line.length() - outstandingLiteralData;
        if (excess > 0) {
            literal = line.left(outstandingLiteralData);
            remainder = line.right(excess);
        } else {
            literal = line;
        }

        // Process the literal data line
        if (literal.length() > 1) {
            qMailLog(ImapData) << objectName() << qPrintable(literal.left(literal.length() - 2));
        }

        _stream.append(literal);
        if (!checkSpace()) {
            _fsm->setState(&_fsm->fullState);
            operationCompleted(_fsm->command(), _fsm->status());
        }

        outstandingLiteralData -= literal.length();
        setLiteralDataRemaining(outstandingLiteralData);

        _fsm->literalResponse(literal);

        if (outstandingLiteralData == 0) {
            // We have received all the literal data
            qMailLog(IMAP) << objectName() << qPrintable(QString("RECV: <%1 literal bytes received>").arg(_stream.length()));
            if (remainder.length() > 2) {
                qMailLog(IMAP) << objectName() << "RECV:" << qPrintable(remainder.left(remainder.length() - 2));
            }

            _unprocessedInput = precedingLiteral();
            if (_fsm->appendLiteralData(precedingLiteral())) {
                // Append this literal data to the preceding line data
                _unprocessedInput.append(_stream.readAll());
            }

            setPrecedingLiteral(QString());

            if (remainder.endsWith("\n")) {
                // Is this trailing part followed by a literal data segment?
                QRegExp literalPattern("\\{(\\d*)\\}\\r?\\n");
                int literalIndex = literalPattern.indexIn(remainder);
                if (literalIndex != -1) {
                    // We are waiting for literal data to complete this line
                    setPrecedingLiteral(_unprocessedInput + remainder.left(literalIndex));
                    setLiteralDataRemaining(literalPattern.cap(1).toInt());
                    _stream.reset();
                }

                // Process the line that contained the literal data
                nextAction(_unprocessedInput + remainder);
                _unprocessedInput.clear();
            } else {
                _unprocessedInput.append(remainder);
            }
        }
    } else {
        if (line.length() > 1) {
            qMailLog(IMAP) << objectName() << "RECV:" << qPrintable(line.left(line.length() - 2));
        }

        // Is this line followed by a literal data segment?
        QRegExp literalPattern("\\{(\\d*)\\}\\r?\\n");
        int literalIndex = literalPattern.indexIn(line);
        if (literalIndex != -1) {
            // We are waiting for literal data to complete this line
            setPrecedingLiteral(line.left(literalIndex));
            setLiteralDataRemaining(literalPattern.cap(1).toInt());
            _stream.reset();
        }

        // Do we have any preceding input to add to this?
        if (!_unprocessedInput.isEmpty()) {
            line.prepend(_unprocessedInput);
            _unprocessedInput.clear();
        }

        // Process this line
        nextAction(line);
    }
}

void ImapProtocol::nextAction(const QString &line)
{
    // We have a completed line to process
    if (!_fsm->tag().isEmpty() && line.startsWith(_fsm->tag())) {
        // Tagged response
        _fsm->setStatus(commandResponse(line));
        if (_fsm->status() != OpOk) {
            _lastError = _fsm->error(line);
            _fsm->log(objectName() + "End:");
            operationCompleted(_fsm->command(), _fsm->status());
            return;
        }
        
        _fsm->taggedResponse(line);
        clearResponse();

        // This state has now completed
        _fsm->stateCompleted();
    } else {
        if ((line.length() > 0) && (line.at(0) == '+')) {
            // Continuation
            _fsm->continuationResponse(line.mid(1).trimmed());
        } else {
            // Untagged response
            _fsm->untaggedResponse(line);
            if (!checkSpace()) {
                _fsm->setState(&_fsm->fullState);
                operationCompleted(_fsm->command(), _fsm->status());
            }
        }
    }
}

QString ImapProtocol::newCommandId()
{
    QString id, out;

    _requestCount++;
    id.setNum( _requestCount );
    out = "a";
    out = out.leftJustified( 4 - id.length(), '0' );
    out += id;
    return out;
}

QString ImapProtocol::commandId( QString in )
{
    int pos = in.indexOf(' ');
    if (pos == -1)
        return "";

    return in.left( pos ).trimmed();
}

OperationStatus ImapProtocol::commandResponse( QString in )
{
    QString old = in;
    int start = in.indexOf( ' ' );
    start = in.indexOf( ' ', start );
    int stop = in.indexOf( ' ', start + 1 );
    if (start == -1 || stop == -1) {
        qMailLog(IMAP) << objectName() << qPrintable("could not parse command response: " + in);
        return OpFailed;
    }

    in = in.mid( start, stop - start ).trimmed().toUpper();
    OperationStatus status = OpFailed;

    if (in == "OK")
        status = OpOk;
    if (in == "NO")
        status = OpNo;
    if (in == "BAD")
        status = OpBad;

    return status;
}

QString ImapProtocol::uid( const QString &identifier )
{
    return messageId(identifier);
}

QString ImapProtocol::url(const QMailMessagePart::Location &location, bool absolute, bool bodyOnly)
{
    QString result;

    QMailMessageId id(location.containingMessageId());
    QMailMessageMetaData metaData(id);
    QMailAccountConfiguration config(metaData.parentAccountId());
    ImapConfiguration imapCfg(config);

    if (metaData.parentAccountId().isValid()) {
        if (absolute) {
            result.append("imap://");

            if (!imapCfg.mailUserName().isEmpty()) {
                result.append(QUrl::toPercentEncoding(imapCfg.mailUserName()));
                result.append('@');
            }
            result.append(imapCfg.mailServer());
            
            if (imapCfg.mailPort() != 143) {
                result.append(':').append(QString::number(imapCfg.mailPort()));
            }
        }
        
        result.append('/');

        if (QMailDisconnected::sourceFolderId(metaData).isValid()) {
            QMailFolder folder(QMailDisconnected::sourceFolderId(metaData));

            result.append(folder.path()).append('/');
        }

        result.append(";uid=").append(uid(metaData.serverUid()));

        if (location.isValid(false)) {
            result.append("/;section=").append(location.toString(false));
        } else if (bodyOnly) {
            result.append("/;section=TEXT");
        }
        
        if (!imapCfg.mailUserName().isEmpty()) {
            result.append(";urlauth=submit+");
            result.append(QUrl::toPercentEncoding(imapCfg.mailUserName()));
        } else {
            qWarning() << "url auth, no user name found";
        }
    }

    return result;
}

// Remove escape characters when receiving from IMAP
QString ImapProtocol::unescapeFolderPath(const QString &path)
{
    QString result(path);
    QString::iterator it = result.begin();
    while (it != result.end()) {
        if ((*it) == '\\') {
            int pos = (it - result.begin());
            result.remove(pos, 1);
            it = result.begin() + pos;
            if (it == result.end()) {
                break;
            }
        }
        ++it;
    }
    return result;
}

// Ensure a string is quoted, if required for IMAP transmission
QString ImapProtocol::quoteString(const QString& input)
{
    // We can't easily catch controls other than those caught by \\s...
    QRegExp atomSpecials("[\\(\\)\\{\\s\\*%\\\\\"\\]]");

    // The empty string must be quoted
    if (input.isEmpty())
        return QString("\"\"");

    if (atomSpecials.indexIn(input) == -1)
        return input;
        
    // We need to quote this string because it is not an atom
    QString result(input);

    QString::iterator it = result.begin();
    while (it != result.end()) {
        // We need to escape any characters specially treated in quotes
        if ((*it) == '\\' || (*it) == '"') {
            int pos = (it - result.begin());
            result.insert(pos, '\\');
            it = result.begin() + (pos + 1);
        }
        ++it;
    }

    return QMail::quoteString(result);
}

QByteArray ImapProtocol::quoteString(const QByteArray& input)
{
    return quoteString(QString(input)).toLatin1();
}

void ImapProtocol::createMail(const QString &uid, const QDateTime &timeStamp, int size, uint flags, const QString &detachedFile, const QStringList& structure)
{
    QMailMessage mail;
    if ( !structure.isEmpty() ) {
        mail = QMailMessage::fromSkeletonRfc2822File( detachedFile );
        bool wellFormed = setMessageContentFromStructure( structure, &mail );

        if (wellFormed && (mail.multipartType() != QMailMessage::MultipartNone)) {
            mail.setStatus( QMailMessage::ContentAvailable, true );
            mail.setSize( size );
        }

        // If we're fetching the structure, this is the first we've seen of this message
        mail.setStatus( QMailMessage::New, true );
    } else {
        // No structure - we're fetching the body of a message we already know about
        mail = QMailMessage::fromRfc2822File( detachedFile );
        mail.setStatus( QMailMessage::ContentAvailable, true );
    }

    if (mail.status() & QMailMessage::ContentAvailable) {
        // ContentAvailable must also imply partial content available
        mail.setStatus( QMailMessage::PartialContentAvailable, true );
    }

    if (flags & MFlag_Seen) {
        mail.setStatus( QMailMessage::ReadElsewhere, true );
        mail.setStatus( QMailMessage::Read, true );
    }
    if (flags & MFlag_Flagged) {
        mail.setStatus( QMailMessage::ImportantElsewhere, true );
        mail.setStatus( QMailMessage::Important, true );
    }
    if (flags & MFlag_Answered) {
        mail.setStatus( QMailMessage::Replied, true );
    }
    if (flags & MFlag_Deleted) {
        mail.setStatus( QMailMessage::Removed, true);
    }

    mail.setMessageType( QMailMessage::Email );
    mail.setSize( size );
    mail.setServerUid( uid.trimmed() );
    mail.setReceivedDate( QMailTimeStamp( timeStamp ) );

    // The file we wrote to is detached, and the mailstore can assume ownership

    emit messageFetched(mail, detachedFile, !structure.isEmpty());

    // Workaround for message buffer file being deleted
    QFileInfo newFile(_fsm->buffer().fileName());
    if (!newFile.exists()) {
        qWarning() << "Unable to find message buffer file";
        _fsm->buffer().detach();
    }
}

void ImapProtocol::createPart(const QString &uid, const QString &section, const QString &detachedFile, int size)
{
    emit dataFetched(uid, section, detachedFile, size);

    // Workaround for message part buffer file being deleted
    QFileInfo newFile(_fsm->buffer().fileName());
    if (!newFile.exists()) {
        qWarning() << "Unable to find message part buffer file";
        _fsm->buffer().detach();
    }
}

void ImapProtocol::createPartHeader(const QString &uid, const QString &section, const QString &detachedFile, int size)
{
    emit partHeaderFetched(uid, section, detachedFile, size);

    // Workaround for message part buffer file being deleted
    QFileInfo newFile(_fsm->buffer().fileName());
    if (!newFile.exists()) {
        qWarning() << "Unable to find message part buffer file";
        _fsm->buffer().detach();
    }
}

#include "imapprotocol.moc"
