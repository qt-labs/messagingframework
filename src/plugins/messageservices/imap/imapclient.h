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

#ifndef IMAPCLIENT_H
#define IMAPCLIENT_H

#include "imapprotocol.h"
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

    void allMessagesReceived();
    void idleNewMailNotification(QMailFolderId);
    void idleFlagsChangedNotification(QMailFolderId);

public slots:
    void transportError(int, const QString &msg);
    void transportError(QMailServiceAction::Status::ErrorCode, const QString &msg);

    void mailboxListed(const QString &, const QString &);
    void messageFetched(QMailMessage& mail, const QString &detachedFilename, bool structureOnly);
    void dataFetched(const QString &uid, const QString &section, const QString &fileName, int size);
    void nonexistentUid(const QString &uid);
    void messageStored(const QString &);
    void messageCopied(const QString &, const QString &);
    void messageCreated(const QMailMessageId &, const QString &);
    void downloadSize(const QString &uid, int);
    void urlAuthorized(const QString &url);
    void folderDeleted(const QMailFolder &folder);
    void folderCreated(const QString &folder);
    void folderRenamed(const QMailFolder &folder, const QString &newName);

protected slots:
    void connectionInactive();
    void commandCompleted(const ImapCommand, const OperationStatus);
    void checkCommandResponse(const ImapCommand, const OperationStatus);
    void commandTransition(const ImapCommand, const OperationStatus);
    void transportStatus(const QString& status);
    void idleOpenRequested(IdleProtocol*);
    void messageBufferFlushed();

private:
    friend class ImapStrategyContextBase;

    void deactivateConnection();
    void retrieveOperationCompleted();

    void operationFailed(int code, const QString &text);
    void operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text);

    void updateFolderCountStatus(QMailFolder *folder);

private:
    static const int InactivityPeriod = 60 * 1000; // 1 minute
    static const int StayAliveCount = 4; // send 4 noops

    QMailAccountConfiguration _config;

    ImapProtocol _protocol;
    QTimer _inactiveTimer;
    int _closeCount;

    QMailFolder _idleFolder;
    bool _waitingForIdle;
    QMailFolderIdList _waitingForIdleFolderIds;
    bool _idlesEstablished;
    bool _qresyncEnabled;

    QMailMessageClassifier _classifier;
    ImapStrategyContext *_strategyContext;

    QMap<QMailFolderId, IdleProtocol*> _monitored;
    QList<QMailMessageBufferFlushCallback*> callbacks;
};

#endif
