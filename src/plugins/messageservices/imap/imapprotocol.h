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

#ifndef IMAPPROTOCOL_H
#define IMAPPROTOCOL_H

#include "imapmailboxproperties.h"
#include <private/longstream_p.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qmailserviceaction.h>
#include <qmailtransport.h>

#ifdef Q_OS_WIN
// Pipe is not a legal filename char in Windows
#define UID_SEPARATOR '#'
#else
#define UID_SEPARATOR '|'
#endif

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
    IMAP_Search_Message,
    IMAP_Append,
    IMAP_UIDSearch,
    IMAP_UIDFetch,
    IMAP_UIDStore,
    IMAP_UIDCopy,
    IMAP_Expunge,
    IMAP_GenUrlAuth,
    IMAP_Close,
    IMAP_Full,
    IMAP_Idle,
    IMAP_Create,
    IMAP_Delete,
    IMAP_Rename,
    IMAP_Enable,
    IMAP_QResync,
    IMAP_FetchFlags,
    IMAP_Noop,
    IMAP_Compress,
    IMAP_Move
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
    MFlag_Unseen    = (1 << 6),
    MFlag_Forwarded = (1 << 7)
};

enum FetchDataItem
{
    F_Rfc822_Size   = (1 << 0),
    F_Rfc822_Header = (1 << 1),
    F_Rfc822        = (1 << 2),
    F_Uid           = (1 << 3),
    F_Flags         = (1 << 4),
    F_BodyStructure = (1 << 5),
    F_SectionHeader = (1 << 6),
    F_BodySection   = (1 << 7),
    F_Date          = (1 << 8)
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

    virtual bool open(const ImapConfiguration& config, qint64 bufferSize = 0);
    void close();
    bool connected() const;
    bool encrypted() const;
    bool inUse() const;
    bool loggingOut() const;

    bool delimiterUnknown() const;

    bool flatHierarchy() const;
    void setFlatHierarchy(bool flat);

    QChar delimiter() const;
    void setDelimiter(QChar delimiter);

    bool compress() const;
    void setCompress(bool comp);

    bool authenticated() const;
    void setAuthenticated(bool auth);

    bool receivedCapabilities() const;
    void setReceivedCapabilities(bool received);

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
    void sendDiscoverDelimiter();
    void sendGenUrlAuth(const QMailMessagePart::Location &location, bool bodyOnly, const QString &mechanism = QString());
    void sendAppend(const QMailFolder &mailbox, const QMailMessageId &message);
    void sendSelect(const QMailFolder &mailbox);
    void sendQResync(const QMailFolder &mailbox);
    void sendExamine(const QMailFolder &mailbox);
    void sendCreate(const QMailFolderId &parentFolderId, const QString &name);
    void sendDelete(const QMailFolder &mailbox);
    void sendRename(const QMailFolder &mailbox, const QString &newname);
    void sendMove(const QMailFolder &mailbox, const QMailFolderId &newParentId);

    /*  Valid in Selected state only */
    void sendSearchMessages(const QMailMessageKey &key, const QString &body, const QMailMessageSortKey &sort, bool count);
    void sendSearch(MessageFlags flags, const QString &range = QString());
    void sendUidSearch(MessageFlags flags, const QString &range = QString());
    void sendFetchFlags(const QString &range, const QString &prefix = QString());
    void sendUidFetch(FetchItemFlags items, const QString &uidList);
    void sendUidFetchSection(const QString &uid, const QString &section, int start, int end);
    void sendUidFetchSectionHeader(const QString &uid, const QString &section);
    void sendUidStore(MessageFlags flags, bool set, const QString &range);
    void sendUidCopy(const QString &range, const QMailFolder &destination);
    void sendExpunge();
    void sendClose();
    void sendEnable(const QString &extensions);
    void sendNoop();
    void sendIdle();
    void sendIdleDone();
    void sendCompress();

    /*  Valid in all states */
    void sendLogout();

    static QString uid(const QString &identifier);

    static QString url(const QMailMessagePart::Location &location, bool absolute, bool bodyOnly);

    static QString quoteString(const QString& input);
    static QByteArray quoteString(const QByteArray& input);
    static QString unescapeFolderPath(const QString &path);

signals:
    void mailboxListed(const QString &flags, const QString &name);
    void messageFetched(QMailMessage& mail, const QString &detachedFilename, bool structureOnly);
    void dataFetched(const QString &uid, const QString &section, const QString &fileName, int size);
    void partHeaderFetched(const QString &uid, const QString &section, const QString &fileName, int size);
    void downloadSize(const QString &uid, int);
    void nonexistentUid(const QString& uid);
    void messageStored(const QString& uid);
    void messageCopied(const QString& copiedUid, const QString& createdUid);
    void messageCreated(const QMailMessageId& id, const QString& uid);
    void urlAuthorized(const QString& url);

    void folderCreated(const QString &folder, bool success);
    void folderDeleted(const QMailFolder &name, bool success);
    void folderRenamed(const QMailFolder &folder, const QString &newPath, bool success);
    void folderMoved(const QMailFolder &folder, const QString &newPath, const QMailFolderId &newParentId, bool success);

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
    void highestModSeq(const QString &);
    void noModSeq();

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
    void createPartHeader(const QString &uid, const QString &section, const QString &file, int size);

    void processResponse(QString line);
    void nextAction(const QString &line);

    void sendData(const QString &cmd, bool maskDebug = false);
    void sendDataLiteral(const QString &cmd, uint length);

    QString newCommandId();
    QString commandId(QString in);
    OperationStatus commandResponse(QString in);
    QString sendCommand(const QString &cmd);
    QString sendCommandLiteral(const QString &cmd, uint length);

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
    bool _flatHierarchy;
    QChar _delimiter;
    bool _authenticated;
    bool _receivedCapabilities;

    static const int MAX_LINES = 30;
    QByteArray _lineBuffer;
};

#endif
