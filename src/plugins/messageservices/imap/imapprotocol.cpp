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

#include "imapprotocol.h"

#include "imapauthenticator.h"
#include "imapconfiguration.h"
#include "imapstructure.h"
#include "integerregion.h"

#include <QApplication>
#include <QTemporaryFile>
#include <qmaillog.h>
#include <longstring_p.h>
#include <qmailaccountconfiguration.h>
#include <qmailmessage.h>
#include <qmailmessageserver.h>
#include <qmailnamespace.h>
#include <qmailtransport.h>

#ifndef QT_NO_OPENSSL
#include <QSslError>
#endif

class ImapTransport : public QMailTransport
{
public:
    ImapTransport(const char* name) :
        QMailTransport(name)
    {
    }

#ifndef QT_NO_OPENSSL
protected:
    virtual bool ignoreCertificateErrors(const QList<QSslError>& errors)
    {
        QMailTransport::ignoreCertificateErrors(errors);

        // Because we can't ask the user (due to string freeze), let's default
        // to ignoring these errors...
        foreach (const QSslError& error, errors)
            if (error.error() == QSslError::NoSslSupport)
                return false;

        return true;
    }
#endif
};

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
    if (uidFormat.indexIn(field) != -1) {
        return messageUid(folderId, uidFormat.cap(1));
    }

    return QString();
}

static QDateTime extractDate(const QString& field)
{
    QRegExp dateFormat("INTERNALDATE *\"([^\"]*)\"");
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

    if (pattern.indexIn(field) == -1)
        return false;

    QString messageFlags = pattern.cap(1);

    flags = 0;
    if (messageFlags.indexOf("\\Seen") != -1)
        flags |= MFlag_Seen;
    if (messageFlags.indexOf("\\Answered") != -1)
        flags |= MFlag_Answered;
    if (messageFlags.indexOf("\\Flagged") != -1)
        flags |= MFlag_Flagged;
    if (messageFlags.indexOf("\\Deleted") != -1)
        flags |= MFlag_Deleted;
    if (messageFlags.indexOf("\\Draft") != -1)
        flags |= MFlag_Draft;
    if (messageFlags.indexOf("\\Recent") != -1)
        flags |= MFlag_Recent;

    return true;
}

static QString token( QString str, QChar c1, QChar c2, int *index )
{
    int start, stop;

    // The strings we're tokenizing use CRLF as the line delimiters - assume that the
    // caller considers the sequence to be atomic.
    if (c1 == QMailMessage::LineFeed)
        c1 = QMailMessage::CarriageReturn;
    start = str.indexOf( c1, *index, Qt::CaseInsensitive );
    if (start == -1)
        return QString::null;

    // Bypass the LF if necessary
    if (c1 == QMailMessage::CarriageReturn)
        start += 1;

    if (c2 == QMailMessage::LineFeed)
        c2 = QMailMessage::CarriageReturn;
    stop = str.indexOf( c2, ++start, Qt::CaseInsensitive );
    if (stop == -1)
        return QString::null;

    // Bypass the LF if necessary
    *index = stop + (c2 == QMailMessage::CarriageReturn ? 2 : 1);

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
    QString result;

    if (flags != 0) {
        if (flags & MFlag_Recent)
            result += " RECENT";
        if (flags & MFlag_Deleted)
            result += " DELETED";
        if (flags & MFlag_Answered)
            result += " ANSWERED";
        if (flags & MFlag_Flagged)
            result += " FLAGGED";
        if (flags & MFlag_Seen)
            result += " SEEN";
        if (flags & MFlag_Unseen)
            result += " UNSEEN";
        if (flags & MFlag_Draft)
            result += " DRAFT";
    }

    return result.trimmed();
}


/* Begin state design pattern related classes */

class ImapState;

class ImapContext
{
public:
    ImapContext(ImapProtocol *protocol) { mProtocol = protocol; }

    void continuation(ImapCommand c, const QString &s) { mProtocol->continuation(c, s); }
    void operationCompleted(ImapCommand c, OperationStatus s) { mProtocol->operationCompleted(c, s); }

    QString sendCommand(const QString &cmd) { return mProtocol->sendCommand(cmd); }
    void sendData(const QString &data) { mProtocol->sendData(data); }

    ImapProtocol *protocol() { return mProtocol; }
    const ImapMailboxProperties &mailbox() { return mProtocol->mailbox(); }

    LongStream &buffer() { return mProtocol->_stream; }
#ifndef QT_NO_OPENSSL
    void switchToEncrypted() { mProtocol->_transport->switchToEncrypted(); mProtocol->clearResponse(); }
#endif
    bool literalResponseCompleted() { return (mProtocol->literalDataRemaining() == 0); }

    // Update the protocol's mailbox properties
    void setMailbox(const QMailFolder &mailbox) { mProtocol->_mailbox = ImapMailboxProperties(mailbox); }
    void setExists(quint32 n) { mProtocol->_mailbox.exists = n; emit mProtocol->exists(n); }
    void setRecent(quint32 n) { mProtocol->_mailbox.recent = n; emit mProtocol->recent(n); }
    void setUnseen(quint32 n) { mProtocol->_mailbox.unseen = n; }
    void setUidValidity(const QString &validity) { mProtocol->_mailbox.uidValidity = validity; emit mProtocol->uidValidity(validity); }
    void setUidNext(quint32 n) { mProtocol->_mailbox.uidNext = n; }
    void setFlags(const QString &flags) { mProtocol->_mailbox.flags = flags; emit mProtocol->flags(flags); }
    void setUidList(const QStringList &uidList) { mProtocol->_mailbox.uidList = uidList; }
    void setMsnList(const QList<uint> &msnList) { mProtocol->_mailbox.msnList = msnList; }
    void setHighestModSeq(const QString &seq) { mProtocol->_mailbox.highestModSeq = seq; mProtocol->_mailbox.noModSeq = false; emit mProtocol->highestModSeq(seq); }
    void setNoModSeq() { mProtocol->_mailbox.noModSeq = true; emit mProtocol->noModSeq(); }

    void createMail(const QString& uid, const QDateTime &timeStamp, int size, uint flags, const QString &file, const QStringList& structure) { mProtocol->createMail(uid, timeStamp, size, flags, file, structure); }
    void createPart(const QString& uid, const QString &section, const QString &file, int size) { mProtocol->createPart(uid, section, file, size); }

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

    virtual void init() { mStatus = OpPending; mTag = QString::null; }

    virtual QString transmit(ImapContext *) { return QString(); }
    virtual void enter(ImapContext *) {}
    virtual void leave(ImapContext *) { init(); }
    
    virtual bool permitsPipelining() const { return false; }

    virtual void continuationResponse(ImapContext *c, const QString &line);
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

void ImapState::continuationResponse(ImapContext *, const QString &line)
{
    qWarning() << "Unexpected continuation response!" << line;
}

void ImapState::untaggedResponse(ImapContext *c, const QString &line)
{
    if (line.indexOf("[ALERT]") != -1)
        qWarning(line.mid(line.indexOf("[ALERT]")).toAscii());
    c->buffer().append(line);
}

void ImapState::taggedResponse(ImapContext *c, const QString &line)
{
    if (line.indexOf("[ALERT]") != -1)
        qWarning(line.mid(line.indexOf("[ALERT]")).toAscii());

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
    // tr string from server - this seems ambitious
    return QObject::tr(line.toAscii());
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

void InitState::untaggedResponse(ImapContext *c, const QString &)
{
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
    if (line.startsWith("* CAPABILITY")) {
        capabilities = line.mid(12).trimmed().split(" ", QString::SkipEmptyParts);
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
#ifndef QT_NO_OPENSSL
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
    virtual void continuationResponse(ImapContext *c, const QString &line);

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

void LoginState::continuationResponse(ImapContext *c, const QString &received)
{
    // The server input is Base64 encoded
    QByteArray challenge = QByteArray::fromBase64(received.toAscii());
    QByteArray response(ImapAuthenticator::getResponse(_config.serviceConfiguration("imap4"), challenge));

    if (!response.isEmpty()) {
        c->sendData(response.toBase64());
    }
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
        c->protocol()->close();
    }

    ImapState::taggedResponse(c, line);
}


class ListState : public ImapState
{
    Q_OBJECT
    
public:
    ListState() : ImapState(IMAP_List, "List") { ListState::init(); }

    bool hasDelimiter() const { return !_delimiter.isNull(); }

    void setParameters(const QString &reference, const QString &mailbox);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *c);
    virtual void untaggedResponse(ImapContext *c, const QString &line);
    virtual void taggedResponse(ImapContext *c, const QString &line);
    
signals:
    void mailboxListed(QString &flags, QString &delimiter, QString &path);

private:
    // The list of reference/mailbox pairs we're listing (via multiple commands), in order
    QList<QPair<QString, QString> > _parameters;
    QChar _delimiter;
};

void ListState::setParameters(const QString &reference, const QString &mailbox)
{
    _parameters.append(qMakePair(reference, mailbox));
}

void ListState::init()
{
    ImapState::init();
    _parameters.clear();
}

QString ListState::transmit(ImapContext *c)
{
    const QPair<QString, QString> &params(_parameters.last());

    if (!params.first.isEmpty()) {
        if (!hasDelimiter()) {
            // We need to wait for a delimiter to be discovered
            return QString();
        }
    } else if (params.second.isEmpty()) {
        // We need to discover the delimiter for this account
        return c->sendCommand("LIST \"\" \"\"");
    }

    QString reference(params.first);
    if (!reference.isEmpty())
        reference.append(_delimiter);

    return c->sendCommand(QString("LIST %1 %2").arg(ImapProtocol::quoteString(reference)).arg(ImapProtocol::quoteString(params.second)));
}

void ListState::leave(ImapContext *)
{
    ImapState::init();
    _parameters.removeFirst();
}

void ListState::untaggedResponse(ImapContext *c, const QString &line)
{
    if (!line.startsWith("* LIST")) {
        ImapState::untaggedResponse(c, line);
        return;
    }

    QString str = line.mid(7);
    QString flags, path, delimiter;
    int pos, index = 0;

    flags = token(str, '(', ')', &index);

    delimiter = token(str, ' ', ' ', &index);
    pos = 0;
    if (token(delimiter, '"', '"', &pos) != QString::null) {
        pos = 0;
        delimiter = token(delimiter, '"', '"', &pos);
    }

    index--;    //to point back to previous => () NIL "INBOX"
    path = token(str, ' ', '\n', &index).trimmed();
    pos = 0;
    if (token(path, '"', '"', &pos) != QString::null) {
        pos = 0;
        path = token(path, '"', '"', &pos);
    }

    if (!hasDelimiter()) {
        // Set the delimiter
        _delimiter = *delimiter.begin();
    }

    if (!path.isEmpty()) {
        emit mailboxListed(flags, delimiter, path);
    }
}

void ListState::taggedResponse(ImapContext *c, const QString &line)
{
    const QPair<QString, QString> &params(_parameters.first());

    if (params.first.isNull() && params.second.isNull()) {
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
    const QPair<QString, QString> &params(_parameters.first());

    return c->sendCommand("GENURLAUTH \"" + params.first + "\" " + params.second);
}

void GenUrlAuthState::leave(ImapContext *)
{
    ImapState::init();
    _parameters.removeFirst();
}

void GenUrlAuthState::untaggedResponse(ImapContext *c, const QString &line)
{
    if (!line.startsWith("* GENURLAUTH")) {
        ImapState::untaggedResponse(c, line);
        return;
    }

    emit urlAuthorized(QMail::unquoteString(line.mid(13).trimmed()));
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

    if (line.indexOf("EXISTS", 0) != -1) {
        int start = 0;
        QString temp = token(line, ' ', ' ', &start);
        quint32 exists = temp.toUInt(&result);
        if (!result)
            exists = 0;
        c->setExists(exists);
    } else if (line.indexOf("RECENT", 0) != -1) {
        int start = 0;
        QString temp = token(line, ' ', ' ', &start);
        quint32 recent = temp.toUInt(&result);
        if (!result)
           recent = 0;
        c->setRecent(recent);
    } else if (line.startsWith("* FLAGS")) {
        int start = 0;
        QString flags = token(line, '(', ')', &start);
        c->setFlags(flags);
    } else if (line.indexOf("UIDVALIDITY", 0) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        c->setUidValidity(temp.mid(12).trimmed());
    } else if (line.indexOf("UIDNEXT", 0) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        QString nextStr = temp.mid( 8 );
        quint32 next = nextStr.toUInt(&result);
        if (!result)
            next = 0;
        c->setUidNext(next);
    } else if (line.indexOf("UNSEEN", 0) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        QString unseenStr = temp.mid( 7 );
        quint32 unseen = unseenStr.toUInt(&result);
        if (!result)
            unseen = 0;
        c->setUnseen(unseen);
    } else if (line.indexOf("HIGHESTMODSEQ", 0) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        c->setHighestModSeq(temp.mid(14).trimmed());
    } else if (line.indexOf("NOMODSEQ", 0) != -1) {
        c->setNoModSeq();
    } else {
        ImapState::untaggedResponse(c, line);
    }
    // TODO consider unilateral EXPUNGE notifications
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


class ExamineState : public SelectState
{
    Q_OBJECT

public:
    ExamineState() : SelectState(IMAP_Examine, "Examine") { ExamineState::init(); }

    virtual QString transmit(ImapContext *c);
};

QString ExamineState::transmit(ImapContext *c)
{
    QString cmd("EXAMINE " + ImapProtocol::quoteString(_mailboxList.last().path()));

    if (c->protocol()->capabilities().contains("CONDSTORE")) {
        cmd.append(" (CONDSTORE)");
    }

    return c->sendCommand(cmd);
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
        while ((temp = token(line, ' ', ' ', &index)) != QString::null) {
            numbers.append(temp.toUInt());
            index--;
        }
        temp = token(line, ' ', '\n', &index);
        if (temp != QString::null)
            numbers.append(temp.toUInt());
        c->setMsnList(numbers);
    } else {
        SelectedState::untaggedResponse(c, line);
    }
}

QString SearchState::error(const QString &line)
{
    return SelectedState::error(line)
        + QLatin1String("\n")
        + QObject::tr( "This server does not provide a complete "
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
        while ((temp = token(line, ' ', ' ', &index)) != QString::null) {
            uidList.append(messageUid(c->mailbox().id, temp));
            index--;
        }
        temp = token(line, ' ', '\n', &index);
        if (temp != QString::null)
            uidList.append(messageUid(c->mailbox().id, temp));
        c->setUidList(uidList);
    } else {
        SelectedState::untaggedResponse(c, line);
    }
}

QString UidSearchState::error(const QString &line)
{
    return SelectedState::error(line)
        + QLatin1String("\n")
        + QObject::tr( "This server does not provide a complete "
                       "IMAP4rev1 implementation." );
}


class UidFetchState : public SelectedState
{
    Q_OBJECT

public:
    UidFetchState() : SelectedState(IMAP_UIDFetch, "UIDFetch") { UidFetchState::init(); }

    void setDataItems(FetchItemFlags flags);
    void setUidList(const QString &uidList);
    void setSection(const QString &uid, const QString &section, int start, int end);

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
    mParameters.clear();
}

void UidFetchState::setDataItems(FetchItemFlags flags)
{
    mParameters.append(FetchParameters());

    mParameters.last().mDataItems = flags;
}

void UidFetchState::setUidList(const QString &uidList)
{
    mParameters.last().mUidList = uidList;
    mParameters.last().mExpectedMessages = IntegerRegion(uidList);
}

void UidFetchState::setSection(const QString &uid, const QString &section, int start, int end)
{
    mParameters.last().mUidList = uid;
    mParameters.last().mSection = section;
    mParameters.last().mStart = start;
    mParameters.last().mEnd = end;
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
            flagStr += ("<" + strStart + "." + strLen + ">");
        }
    }

    if (!flagStr.isEmpty())
        flagStr = "(" + flagStr.trimmed() + ")";

    return c->sendCommand(QString("UID FETCH %1 %2").arg(params.mUidList).arg(flagStr));
}

void UidFetchState::leave(ImapContext *)
{
    SelectedState::init();
    mParameters.removeFirst();
}

void UidFetchState::untaggedResponse(ImapContext *c, const QString &line)
{
    QString str = line;
    QRegExp fetchResponsePattern("\\*\\s+\\d+\\s+(\\w+)");
    if ((fetchResponsePattern.indexIn(str) == 0) && (fetchResponsePattern.cap(1).compare("FETCH", Qt::CaseInsensitive) == 0)) {
        FetchParameters &fp(mParameters.first());

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

            if (fp.mNewMsgFlags & MFlag_Deleted) {
                // This message has been deleted - there is no more information to process
                emit nonexistentUid(fp.mNewMsgUid);
                return;
            }

            if (fp.mDataItems & F_Date) {
                fp.mDate = extractDate(str);
            }

            if (fp.mDataItems & F_Rfc822_Size) {
                fp.mNewMsgSize = extractSize(str);
            }

            if (!c->literalResponseCompleted())
                return;

            // All message data should be retrieved at this point - create new mail/part
            if (fp.mDataItems & F_BodyStructure) {
                fp.mNewMsgStructure = extractStructure(str);
            }

            if (fp.mDataItems & F_BodySection) {
                if (fp.mDetachedFile.isEmpty()) {
                    // The buffer has not been detached to a file yet
                    fp.mDetachedSize = c->buffer().length();
                    fp.mDetachedFile = c->buffer().detach();
                }
                c->createPart(fp.mNewMsgUid, fp.mSection, fp.mDetachedFile, fp.mDetachedSize);
            } else {
                if (fp.mNewMsgSize == 0) {
                    fp.mNewMsgSize = fp.mDetachedSize;
                }
                c->createMail(fp.mNewMsgUid, fp.mDate, fp.mNewMsgSize, fp.mNewMsgFlags, fp.mDetachedFile, fp.mNewMsgStructure);
            }
        }
    } else {
        SelectedState::untaggedResponse(c, line);
    }
}

void UidFetchState::taggedResponse(ImapContext *c, const QString &line)
{
    if (status() == OpOk) {
        FetchParameters &fp(mParameters.first());

        IntegerRegion missingUids = fp.mExpectedMessages.subtract(fp.mReceivedMessages);
        foreach(const QString &uid, missingUids.toStringList()) {
            qMailLog(IMAP) << "Message not found " << uid;
            emit nonexistentUid(messageUid(c->mailbox().id, uid));
        } 
    } 

    SelectedState::taggedResponse(c, line);
}

void UidFetchState::literalResponse(ImapContext *c, const QString &line)
{
    if (!c->literalResponseCompleted()) {
        FetchParameters &fp(mParameters.first());
        ++fp.mReadLines;

        if ((fp.mDataItems & F_Rfc822) || (fp.mDataItems & F_BodySection)) {
            fp.mMessageLength += line.length();

            if (fp.mReadLines > MAX_LINES) {
                fp.mReadLines = 0;
                emit downloadSize(fp.mNewMsgUid, fp.mMessageLength);
            }
        }
    }
}

bool UidFetchState::appendLiteralData(ImapContext *c, const QString &preceding)
{
    FetchParameters &fp(mParameters.first());

    QRegExp pattern;
    if (fp.mDataItems & F_Rfc822_Header) {
        // If the literal is the header data, keep it in the buffer file
        pattern = QRegExp("RFC822\\.HEADER ");
    } else {
        // If the literal is the body data, keep it in the buffer file
        pattern = QRegExp("BODY\\[\\S*\\] ");
    }

    int index = preceding.lastIndexOf(pattern);
    if (index != -1) {
        if ((index + pattern.cap(0).length()) == preceding.length()) {
            // Detach the retrieved data to a file we have ownership of
            fp.mDetachedSize = c->buffer().length();
            fp.mDetachedFile = c->buffer().detach();
            return false;
        }
    }

    return true;
}


class UidStoreState : public SelectedState
{
    Q_OBJECT

public:
    UidStoreState() : SelectedState(IMAP_UIDStore, "UIDStore") { UidStoreState::init(); }

    void setParameters(MessageFlags flags, const QString &range);

    virtual bool permitsPipelining() const { return true; }
    virtual void init();
    virtual QString transmit(ImapContext *c);
    virtual void leave(ImapContext *);
    virtual void taggedResponse(ImapContext *c, const QString &line);

signals:
    void messageStored(const QString &uid);

private:
    // The list of flags/range pairs we're storing (via multiple commands), in order
    QList<QPair<MessageFlags, QString> > _parameters;
};

void UidStoreState::setParameters(MessageFlags flags, const QString &range)
{
    _parameters.append(qMakePair(flags, range));
}

void UidStoreState::init()
{
    SelectedState::init();
    _parameters.clear();
}

QString UidStoreState::transmit(ImapContext *c)
{
    const QPair<MessageFlags, QString> &params(_parameters.last());

    // Note that \Recent flag is ignored as only the server is allowed to modify that flag 
    QString flagStr = "+FLAGS.SILENT (";
    if (params.first & MFlag_Deleted)
        flagStr += "\\Deleted "; // No tr
    if (params.first & MFlag_Answered)
        flagStr += "\\Answered "; // No tr
    if (params.first & MFlag_Flagged)
        flagStr += "\\Flagged "; // No tr
    if (params.first & MFlag_Seen)
        flagStr += "\\Seen "; // No tr
    if (params.first & MFlag_Draft)
        flagStr += "\\Draft "; // No tr
    flagStr = flagStr.trimmed() + ")";

    return c->sendCommand(QString("UID STORE %1 %2").arg(params.second).arg(flagStr));
}

void UidStoreState::leave(ImapContext *)
{
    SelectedState::init();
    _parameters.removeFirst();
}

void UidStoreState::taggedResponse(ImapContext *c, const QString &line)
{
    if (status() == OpOk) {
        const QPair<MessageFlags, QString> &params(_parameters.first());

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

    return c->sendCommand(QString("UID COPY %1 %2").arg(params.first).arg(params.second.path()));
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
        QRegExp copyuidResponsePattern("COPYUID (\\w+) (\\w+) (\\w+)");
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
        // After a close, we no longer have a selectd mailbox
        c->setMailbox(QMailFolder());
    }

    ImapState::taggedResponse(c, line);
}


class FullState : public ImapState
{
    Q_OBJECT

public:
    FullState() : ImapState(IMAP_Full, "Full") { setStatus(OpFailed); }
};


class IdleState : public ImapState
{
    Q_OBJECT

public:
    IdleState() : ImapState(IMAP_Idle, "Idle") {}

    void done(ImapContext *c);

    virtual QString transmit(ImapContext *c);
    virtual void continuationResponse(ImapContext *c, const QString &line);
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

void IdleState::continuationResponse(ImapContext *c, const QString &)
{
    c->continuation(command(), QString("idling"));
}

void IdleState::untaggedResponse(ImapContext *c, const QString &line)
{
    QString str = line;
    QRegExp idleResponsePattern("\\*\\s+\\d+\\s+(\\w+)");
    if (idleResponsePattern.indexIn(str) == 0) {
        // Treat this event as a continuation point
       if (idleResponsePattern.cap(1).compare("EXISTS", Qt::CaseInsensitive) == 0) {
             c->continuation(command(), QString("newmail"));
        } else if (idleResponsePattern.cap(1).compare("FETCH", Qt::CaseInsensitive) == 0) {
            c->continuation(command(), QString("flagschanged"));
        }
    }
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
    SelectState selectState;
    ExamineState examineState;
    SearchState searchState;
    UidSearchState uidSearchState;
    UidFetchState uidFetchState;
    UidStoreState uidStoreState;
    UidCopyState uidCopyState;
    ExpungeState expungeState;
    CloseState closeState;
    FullState fullState;
    IdleState idleState;

    void continuationResponse(const QString &line) { state()->continuationResponse(this, line); }
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
      _literalDataRemaining(0)
{
    connect(&_incomingDataTimer, SIGNAL(timeout()),
            this, SLOT(incomingData()));
    connect(&_fsm->listState, SIGNAL(mailboxListed(QString &, QString &, QString &)),
            this, SIGNAL(mailboxListed(QString &, QString &, QString &)));
    connect(&_fsm->genUrlAuthState, SIGNAL(urlAuthorized(QString)),
            this, SIGNAL(urlAuthorized(QString)));
    connect(&_fsm->uidFetchState, SIGNAL(downloadSize(QString, int)), 
            this, SIGNAL(downloadSize(QString, int)));
    connect(&_fsm->uidFetchState, SIGNAL(nonexistentUid(QString)), 
            this, SIGNAL(nonexistentUid(QString)));
    connect(&_fsm->uidStoreState, SIGNAL(messageStored(QString)), 
            this, SIGNAL(messageStored(QString)));
    connect(&_fsm->uidCopyState, SIGNAL(messageCopied(QString, QString)), 
            this, SIGNAL(messageCopied(QString, QString)));
}

ImapProtocol::~ImapProtocol()
{
    delete _transport;
}

bool ImapProtocol::open( const ImapConfiguration& config )
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
    _precedingLiteral = QString();

    _mailbox = ImapMailboxProperties();

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

    _transport->open( config.mailServer(), config.mailPort(), static_cast<QMailTransport::EncryptType>(config.mailEncryption()) );

    return true;
}

void ImapProtocol::close()
{
    if (_transport)
        _transport->close();
    _stream.reset();
    _fsm->reset();

    _mailbox = ImapMailboxProperties();
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

void ImapProtocol::sendIdle()
{
    _fsm->setState(&_fsm->idleState);
}

void ImapProtocol::sendIdleDone()
{
    if (_fsm->state() == &_fsm->idleState)
        _fsm->idleState.done(_fsm);
}

void ImapProtocol::sendList( const QMailFolder &reference, const QString &mailbox )
{
    QString path;
    if (!reference.path().isEmpty()) {
        path = reference.path();
    }

    if (!path.isEmpty()) {
        // If we don't have the delimiter for this account, we need to discover it
        if (!_fsm->listState.hasDelimiter()) {
            _fsm->listState.setParameters(QString(), QString());
            _fsm->setState(&_fsm->listState);
        }
    }

    // Now request the actual list
    _fsm->listState.setParameters(path, mailbox);
    _fsm->setState(&_fsm->listState);
}

void ImapProtocol::sendGenUrlAuth(const QMailMessagePart::Location &location, bool bodyOnly, const QString &mechanism)
{
    QString dataUrl(url(location, bodyOnly));
    dataUrl.append(";urlauth=anonymous");

    _fsm->genUrlAuthState.setUrl(dataUrl, mechanism);
    _fsm->setState(&_fsm->genUrlAuthState);
}

void ImapProtocol::sendSelect(const QMailFolder &mailbox)
{
    _fsm->selectState.setMailbox(mailbox);
    _fsm->setState(&_fsm->selectState);
}

void ImapProtocol::sendExamine(const QMailFolder &mailbox)
{
    _fsm->examineState.setMailbox(mailbox);
    _fsm->setState(&_fsm->examineState);
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

void ImapProtocol::sendUidFetch(FetchItemFlags items, const QString &uidList)
{
    _fsm->uidFetchState.setDataItems(items | F_Flags);
    _fsm->uidFetchState.setUidList(uidList);
    _fsm->setState(&_fsm->uidFetchState);
}

void ImapProtocol::sendUidFetchSection(const QString &uid, const QString &section, int start, int end)
{
    _fsm->uidFetchState.setDataItems(F_Uid | F_BodySection);
    _fsm->uidFetchState.setSection(uid, section, start, end);
    _fsm->setState(&_fsm->uidFetchState);
}

void ImapProtocol::sendUidStore(MessageFlags flags, const QString &range)
{
    _fsm->uidStoreState.setParameters(flags, range);
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

void ImapProtocol::connected(QMailTransport::EncryptType encryptType)
{
#ifndef QT_NO_OPENSSL
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

void ImapProtocol::sendData(const QString &cmd)
{
    const QByteArray &output(cmd.toAscii());

    QDataStream &out(_transport->stream());
    out.writeRawData(output.data(), output.length());
    out.writeRawData("\r\n", 2);

    QString logCmd(cmd);
    QRegExp loginExp("^[^\\s]+\\sLOGIN\\s[^\\s]+\\s");
    if (loginExp.indexIn(cmd) != -1) {
        logCmd = cmd.left(loginExp.matchedLength()) + "<password hidden>";
    }
    qMailLog(IMAP) << objectName() << "SEND:" << qPrintable(logCmd);
}

QString ImapProtocol::sendCommand(const QString &cmd)
{
    QString tag = newCommandId();

    _stream.reset();
    sendData(tag + " " + cmd);

    return tag;
}

void ImapProtocol::incomingData()
{
    int readLines = 0;
    while (_transport->canReadLine()) {
        processResponse(_transport->readLine());

        readLines++;
        if (readLines >= MAX_LINES) {
            _incomingDataTimer.start(0);
            return;
        }
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
        _lastError += LongStream::errorMessage( "\n" );
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

            if (remainder.endsWith("\r\n")) {
                // Is this trailing part followed by a literal data segment?
                QRegExp literalPattern("\\{(\\d*)\\}\\r\\n");
                int literalIndex = literalPattern.indexIn(remainder);
                if (literalIndex != -1) {
                    // We are waiting for literal data to complete this line
                    setPrecedingLiteral(_unprocessedInput + remainder.left(literalIndex));
                    setLiteralDataRemaining(literalPattern.cap(1).toInt());
                    _stream.reset();
                }

                // Process the line that contained the literal data
                nextAction(_unprocessedInput + remainder);
                _unprocessedInput = QString();
            } else {
                _unprocessedInput.append(remainder);
            }
        }
    } else {
        if (line.length() > 1) {
            qMailLog(IMAP) << objectName() << "RECV:" << qPrintable(line.left(line.length() - 2));
        }

        // Is this line followed by a literal data segment?
        QRegExp literalPattern("\\{(\\d*)\\}\\r\\n");
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
            _unprocessedInput = QString();
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

QString ImapProtocol::url(const QMailMessagePart::Location &location, bool bodyOnly)
{
    QString result("imap://");

    QMailMessageId id(location.containingMessageId());
    QMailMessageMetaData metaData(id);

    if (metaData.parentAccountId().isValid()) {
        QMailAccountConfiguration config(metaData.parentAccountId());
        ImapConfiguration imapCfg(config);

        if (!imapCfg.mailUserName().isEmpty()) {
            result.append(imapCfg.mailUserName()).append('@');
        }

        result.append(imapCfg.mailServer());
        if (imapCfg.mailPort() != 143) {
            result.append(':').append(QString::number(imapCfg.mailPort()));
        }
        
        result.append('/');

        if (metaData.parentFolderId().isValid()) {
            QMailFolder folder(metaData.parentFolderId());

            result.append(folder.path()).append('/');
        }

        result.append(";uid=").append(uid(metaData.serverUid()));

        if (location.isValid(false)) {
            result.append("/;section=").append(location.toString(false));
        } else if (bodyOnly) {
            result.append("/;section=TEXT");
        }
    }

    return result;
}

// Ensure a string is quoted, if required for IMAP transmission
QString ImapProtocol::quoteString(const QString& input)
{
    // We can't easily catch controls other than those caught by \\s...
    static const QRegExp atomSpecials("[\\(\\)\\{\\s\\*%\\\\\"\\]]");

    // The empty string must be quoted
    if (input.isEmpty())
        return QString("\"\"");

    if (atomSpecials.indexIn(input) == -1)
        return input;
        
    // We need to quote this string because it is not an atom
    QString result(input);

    QString::iterator begin = result.begin(), it = begin;
    while (it != result.end()) {
        // We need to escape any characters specially treated in quotes
        if ((*it) == '\\' || (*it) == '"') {
            int pos = (it - begin);
            result.insert(pos, '\\');
            it = result.begin() + (pos + 1);
        }
        ++it;
    }

    return QMail::quoteString(result);
}

QByteArray ImapProtocol::quoteString(const QByteArray& input)
{
    return quoteString(QString(input)).toAscii();
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

void ImapProtocol::createMail(const QString &uid, const QDateTime &timeStamp, int size, uint flags, const QString &detachedFile, const QStringList& structure)
{
    QMailMessage mail = QMailMessage::fromRfc2822File( detachedFile );
    if ( !structure.isEmpty() ) {
        setMessageContentFromStructure( structure, &mail );

        if (mail.multipartType() != QMailMessage::MultipartNone) {
            mail.setStatus( QMailMessage::ContentAvailable, true );
            mail.setSize( size );

            // See if any of the parts are attachments
            if (hasAttachments(mail)) {
                mail.setStatus( QMailMessage::HasAttachments, true );
            }
        }

        // If we're fetching the structure, this is the first we've seen of this message
        mail.setStatus( QMailMessage::New, true );
    } else {
        // No structure - we're fetching the body of a message we already know about
        mail.setStatus( QMailMessage::ContentAvailable, true );
        mail.setStatus( QMailMessage::New, false );
    }

    if (mail.status() & QMailMessage::ContentAvailable) {
        // ContentAvailable must also imply partial content available
        mail.setStatus( QMailMessage::PartialContentAvailable, true );
    }

    if (flags & MFlag_Seen) {
        mail.setStatus( QMailMessage::ReadElsewhere, true );
    }
    if (flags & MFlag_Answered) {
        mail.setStatus( QMailMessage::Replied, true );
    }

    mail.setStatus( QMailMessage::Incoming, true );

    mail.setMessageType( QMailMessage::Email );
    mail.setSize( size );
    mail.setServerUid( uid.trimmed() );
    mail.setReceivedDate( QMailTimeStamp( timeStamp ) );

    // The file we wrote to is detached, and the mailstore can assume ownership
    mail.setCustomField( "qtopiamail-detached-filename", detachedFile );

    emit messageFetched(mail);

    // Remove the detached file if it is still present
    QFile::remove(detachedFile);
}

void ImapProtocol::createPart(const QString &uid, const QString &section, const QString &detachedFile, int size)
{
    emit dataFetched(uid, section, detachedFile, size);

    // Remove the detached file if it is still present
    QFile::remove(detachedFile);
}

#include "imapprotocol.moc"
