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

#ifndef MAILMESSAGECLIENT_H
#define MAILMESSAGECLIENT_H

#include "qmailmessageserver.h"
#include <QObject>
#include <qmailipc.h>

// The back-end corresponding to the front-end in QMailMessageServer
class MailMessageClient : public QObject
{
    Q_OBJECT

    friend class MessageServer;

public:
    MailMessageClient(QObject* parent);
    ~MailMessageClient();

private:
    // Disallow copying
    MailMessageClient(const MailMessageClient&);
    void operator=(const MailMessageClient&);

signals:
    void newCountChanged(const QMailMessageCountMap&);
    void acknowledgeNewMessages(const QMailMessageTypeList&);

    void transmitMessages(quint64, const QMailAccountId &accountId);

    void retrieveFolderList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);

    void retrieveMessages(quint64, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(quint64, const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(quint64, const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(quint64, const QMailMessagePart::Location &partLocation, uint minimum);

    void retrieveAll(quint64, const QMailAccountId &accountId);
    void exportUpdates(quint64, const QMailAccountId &accountId);

    void synchronize(quint64, const QMailAccountId &accountId);

    void deleteMessages(quint64, const QMailMessageIdList&, QMailStore::MessageRemovalOption);

    void copyMessages(quint64, const QMailMessageIdList&, const QMailFolderId&);
    void moveMessages(quint64, const QMailMessageIdList&, const QMailFolderId&);
    void flagMessages(quint64, const QMailMessageIdList&, quint64 setMask, quint64 unsetMask);

    void createFolder(quint64, const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    void renameFolder(quint64, const QMailFolderId &folderId, const QString &name);
    void deleteFolder(quint64, const QMailFolderId &folderId);

    void cancelTransfer(quint64);

    void searchMessages(quint64, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);

    void cancelSearch(quint64);

    void shutdown();

    void protocolRequest(quint64, const QMailAccountId &accountId, const QString &request, const QVariant &data);

    void activityChanged(quint64, QMailServiceAction::Activity);
    void connectivityChanged(quint64, QMailServiceAction::Connectivity);
    void statusChanged(quint64, const QMailServiceAction::Status);
    void progressChanged(quint64, uint, uint);

    void retrievalCompleted(quint64);

    void messagesTransmitted(quint64, const QMailMessageIdList&);
    void transmissionCompleted(quint64);

    void messagesDeleted(quint64, const QMailMessageIdList&);
    void messagesCopied(quint64, const QMailMessageIdList&);
    void messagesMoved(quint64, const QMailMessageIdList&);
    void messagesFlagged(quint64, const QMailMessageIdList&);
    
    void folderCreated(quint64, const QMailFolderId&);
    void folderRenamed(quint64, const QMailFolderId&);
    void folderDeleted(quint64, const QMailFolderId&);
    
    void storageActionCompleted(quint64);

    void matchingMessageIds(quint64, const QMailMessageIdList&);
    void searchCompleted(quint64);

    void protocolResponse(quint64, const QString &response, const QVariant &data);
    void protocolRequestCompleted(quint64);

private:
    QCopAdaptor* adaptor;
};

#endif
