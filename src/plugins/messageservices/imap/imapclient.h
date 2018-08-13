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

#ifndef IMAPCLIENT_H
#define IMAPCLIENT_H

#include "imapprotocol.h"
#include "qtimer.h"
#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>
#include <qlist.h>
#include <qtimer.h>
#include <qmailaccountconfiguration.h>
#include <qmailfolder.h>
#include <qmailmessage.h>
#include <qmailmessageclassifier.h>
#include <qmailmessageserver.h>


class ImapStrategy;
class ImapStrategyContext;
class IdleProtocol;
class QMailMessageBufferFlushCallback;

class ImapClient : public QObject
{
    Q_OBJECT

public:
    ImapClient(QObject* parent);
    ~ImapClient();

    void setAccount(const QMailAccountId& accountId);
    QMailAccountId account() const;
    void requestRapidClose() { _requestRapidClose = true; } // Close connection ASAP, unless interactive checking occurred recently

    void newConnection();
    void cancelTransfer(QMailServiceAction::Status::ErrorCode code, const QString &text);
    void closeConnection();

    ImapStrategyContext *strategyContext();

    ImapStrategy *strategy() const;
    void setStrategy(ImapStrategy *strategy);

    QMailFolderId mailboxId(const QString &path) const;
    QMailFolderIdList mailboxIds() const;

    QStringList serverUids(const QMailFolderId &folderId) const;
    QStringList serverUids(const QMailFolderId &folderId, quint64 messageStatusFilter, bool set = true) const;
    QStringList serverUids(QMailMessageKey key) const;
    QMailMessageKey messagesKey(const QMailFolderId &folderId) const;
    QMailMessageKey trashKey(const QMailFolderId &folderId) const;
    QStringList deletedMessages(const QMailFolderId &folderId) const;

    bool idlesEstablished();
    void idling(const QMailFolderId &id);
    QMailFolderIdList configurationIdleFolderIds();
    void monitor(const QMailFolderIdList &mailboxIds);
    void removeAllFromBuffer(QMailMessage *message);
    int pushConnectionsReserved() { return _pushConnectionsReserved; }
    void setPushConnectionsReserved(int reserved) { _pushConnectionsReserved = reserved; }
    int idleRetryDelay() const { return _idleRetryDelay; }
    void setIdleRetryDelay(int delay) { _idleRetryDelay = delay; }

signals:
    void errorOccurred(int, const QString &);
    void errorOccurred(QMailServiceAction::Status::ErrorCode, const QString &);
    void updateStatus(const QString &);
    void restartPushEmail();

    void progressChanged(uint, uint);
    void retrievalCompleted();

    void messageCopyCompleted(QMailMessage &message, const QMailMessage &original);

    void messageActionCompleted(const QString &uid);

    void matchingMessageIds(const QMailMessageIdList &messages);
    void remainingMessagesCount(uint);
    void messagesCount(uint);

    void allMessagesReceived();
    void idleNewMailNotification(QMailFolderId);
    void idleFlagsChangedNotification(QMailFolderId);
    void sessionError();

public slots:
    void transportError(int, const QString &msg);
    void transportError(QMailServiceAction::Status::ErrorCode, const QString &msg);

    void mailboxListed(const QString &, const QString &);
    void messageFetched(QMailMessage& mail, const QString &detachedFilename, bool structureOnly);
    void dataFetched(const QString &uid, const QString &section, const QString &fileName, int size);
    void partHeaderFetched(const QString &uid, const QString &section, const QString &fileName, int size);
    void nonexistentUid(const QString &uid);
    void messageStored(const QString &);
    void messageCopied(const QString &, const QString &);
    void messageCreated(const QMailMessageId &, const QString &);
    void downloadSize(const QString &uid, int);
    void urlAuthorized(const QString &url);
    void folderDeleted(const QMailFolder &folder, bool success);
    void folderCreated(const QString &folder, bool success);
    void folderRenamed(const QMailFolder &folder, const QString &newName, bool success);
    void folderMoved(const QMailFolder &folder, const QString &newName, const QMailFolderId &newParentId, bool success);

protected slots:
    void connectionInactive();
    void commandCompleted(const ImapCommand, const OperationStatus);
    void checkCommandResponse(const ImapCommand, const OperationStatus);
    void commandTransition(const ImapCommand, const OperationStatus);
    void transportStatus(const QString& status);
    void idleOpenRequested();
    void messageBufferFlushed();

private:
    friend class ImapStrategyContextBase;

    void deactivateConnection();
    void retrieveOperationCompleted();

    void operationFailed(int code, const QString &text);
    void operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text);

    void updateFolderCountStatus(QMailFolder *folder);

    static const int MaxTimeBeforeNoop = 60 * 1000; // 1 minute (this must be >= 1ms)
    QMailAccountConfiguration _config;

    ImapProtocol _protocol;
    QTimer _inactiveTimer;
    int _closeCount;

    bool _waitingForIdle;
    QMailFolderIdList _waitingForIdleFolderIds;
    bool _idlesEstablished;
    bool _qresyncEnabled;
    bool _requestRapidClose;
    bool _rapidClosing;
    int _idleRetryDelay; // Try to restablish IDLE state
    enum IdleRetryDelay { InitialIdleRetryDelay = 30 }; //seconds

    QMailMessageClassifier _classifier;
    ImapStrategyContext *_strategyContext;

    QMap<QMailFolderId, IdleProtocol*> _monitored;
    QList<QMailMessageBufferFlushCallback*> callbacks;
    QVector<QMailMessage*> _bufferedMessages;
    int _pushConnectionsReserved;

    QMap<QMailMessageId,QString> detachedTempFiles;
};

#endif
