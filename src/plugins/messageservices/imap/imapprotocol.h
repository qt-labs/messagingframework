/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef IMAPPROTOCOL_H
#define IMAPPROTOCOL_H

#include "imapmailboxproperties.h"
#include <longstream_p.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qmailserviceaction.h>
#include <qmailtransport.h>

class QMailMessage;

enum ImapCommand
{
    IMAP_Unconnected = 0,
    IMAP_Init,
    IMAP_Capability,
    IMAP_Idle_Continuation,
    IMAP_StartTLS,
    IMAP_Login,
    IMAP_Logout,
    IMAP_List,
    IMAP_Select,
    IMAP_Examine,
    IMAP_Search,
    IMAP_UIDSearch,
    IMAP_UIDFetch,
    IMAP_UIDStore,
    IMAP_UIDCopy,
    IMAP_Expunge,
    IMAP_Close,
    IMAP_Full,
    IMAP_Idle
};

enum MessageFlag
{
    MFlag_All       = 0, // Not a true flag
    MFlag_Seen      = (1 << 0),
    MFlag_Answered  = (1 << 1),
    MFlag_Flagged   = (1 << 2),
    MFlag_Deleted   = (1 << 3),
    MFlag_Draft     = (1 << 4),
    MFlag_Recent    = (1 << 5),
    MFlag_Unseen    = (1 << 6)
};

typedef uint MessageFlags;

enum FetchDataItem
{
    F_Rfc822_Size   = (1 << 0),
    F_Rfc822_Header = (1 << 1),
    F_Rfc822        = (1 << 2),
    F_Uid           = (1 << 3),
    F_Flags         = (1 << 4),
    F_BodyStructure = (1 << 5),
    F_BodySection   = (1 << 6),
    F_Date          = (1 << 7)
};

typedef uint FetchItemFlags;

enum OperationStatus
{
    OpPending = 0,
    OpFailed,
    OpOk,
    OpNo,
    OpBad,
};

class LongStream;
class Email;
class ImapConfiguration;
class ImapTransport;
class ImapContextFSM;
class QMailAccountConfiguration;

class ImapProtocol: public QObject
{
    Q_OBJECT

public:
    ImapProtocol();
    ~ImapProtocol();

    virtual bool open(const ImapConfiguration& config);
    void close();
    bool connected() const;
    bool encrypted() const;
    bool inUse() const;
    bool loggingOut() const;

    QString lastError() const { return _lastError; };

    const QStringList &capabilities() const;
    void setCapabilities(const QStringList &);

    bool supportsCapability(const QString& name) const;

    const ImapMailboxProperties &mailbox() const { return _mailbox; }

    /*  Valid in non-authenticated state only    */
    void sendCapability();
    void sendStartTLS();
    void sendLogin(const QMailAccountConfiguration &config);

    /* Valid in authenticated state only    */
    void sendList(const QMailFolder &reference, const QString &mailbox);
    void sendSelect(const QMailFolder &mailbox);
    void sendExamine(const QMailFolder &mailbox);

    /*  Valid in Selected state only */
    void sendSearch(MessageFlags flags, const QString &range = QString());
    void sendUidSearch(MessageFlags flags, const QString &range = QString());
    void sendUidFetch(FetchItemFlags items, const QString &uidList);
    void sendUidFetchSection(const QString &uid, const QString &section, int start, int end);
    void sendUidStore(MessageFlags flags, const QString &range);
    void sendUidCopy(const QString &range, const QMailFolder &destination);
    void sendExpunge();
    void sendClose();
    void sendIdle();
    void sendIdleDone();

    /*  Valid in all states */
    void sendLogout();

    static QString uid(const QString &identifier);

    static QString quoteString(const QString& input);
    static QByteArray quoteString(const QByteArray& input);

signals:
    void mailboxListed(QString &flags, QString &delimiter, QString &name);
    void messageFetched(QMailMessage& mail);
    void dataFetched(const QString &uid, const QString &section, const QString &fileName, int size);
    void downloadSize(const QString &uid, int);
    void nonexistentUid(const QString& uid);
    void messageStored(const QString& uid);
    void messageCopied(const QString& copiedUid, const QString& createdUid);

    void continuationRequired(ImapCommand, const QString &);
    void completed(ImapCommand, OperationStatus);
    void updateStatus(const QString &);

    void connectionError(int status, const QString &msg);
    void connectionError(QMailServiceAction::Status::ErrorCode status, const QString &msg);

    // Possibly unilateral notifications related to currently selected folder
    void exists(int);
    void recent(int);
    void uidValidity(const QString &);
    void flags(const QString &);

protected slots:
    void connected(QMailTransport::EncryptType encryptType);
    void errorHandling(int status, QString msg);
    void incomingData();

private:
    friend class ImapContext;

    void clearResponse();

    int literalDataRemaining() const;
    void setLiteralDataRemaining(int literalDataRemaining);

    QString precedingLiteral() const;
    void setPrecedingLiteral(const QString &line);

    void continuation(ImapCommand, const QString &);
    void operationCompleted(ImapCommand, OperationStatus);

    bool checkSpace();

    void createMail(const QString &uid, const QDateTime &timeStamp, int size, uint flags, const QString &file, const QStringList& structure);
    void createPart(const QString &uid, const QString &section, const QString &file, int size);

    void processResponse(QString line);
    void nextAction(const QString &line);

    void sendData(const QString &cmd);

    QString newCommandId();
    QString commandId(QString in);
    OperationStatus commandResponse(QString in);
    QString sendCommand(const QString &cmd);

    void parseChange();

private:
    ImapContextFSM *_fsm;
    ImapTransport *_transport;
    LongStream _stream;

    QStringList _capabilities;
    ImapMailboxProperties _mailbox;

    QStringList _errorList;
    QString _lastError;
    int _requestCount;

    int _literalDataRemaining;
    QString _precedingLiteral;

    QString _unprocessedInput;

    QTimer _incomingDataTimer;

    static const int MAX_LINES = 30;
};

#endif
