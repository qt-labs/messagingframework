/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAILMESSAGECLIENT_H
#define MAILMESSAGECLIENT_H

#include "qmailmessageserver.h"
#include <QObject>
#include <qmailipc.h>
#include <qcopadaptor.h>

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
    void transmitMessage(quint64, const QMailMessageId &messageId);

    void retrieveFolderList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);
    void retrieveMessageLists(quint64, const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort);
    void retrieveNewMessages(quint64, const QMailAccountId &accountId, const QMailFolderIdList &folderIds);

    void createStandardFolders(quint64, const QMailAccountId &accountId);

    void retrieveMessages(quint64, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(quint64, const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(quint64, const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(quint64, const QMailMessagePart::Location &partLocation, uint minimum);

    void retrieveAll(quint64, const QMailAccountId &accountId);
    void exportUpdates(quint64, const QMailAccountId &accountId);

    void synchronize(quint64, const QMailAccountId &accountId);

    void onlineDeleteMessages(quint64, const QMailMessageIdList&, QMailStore::MessageRemovalOption);

    void onlineCopyMessages(quint64, const QMailMessageIdList&, const QMailFolderId&);
    void onlineMoveMessages(quint64, const QMailMessageIdList&, const QMailFolderId&);
    void onlineFlagMessagesAndMoveToStandardFolder(quint64, const QMailMessageIdList&, quint64 setMask, quint64 unsetMask);
    void addMessages(quint64, const QString &filename);
    void addMessages(quint64, const QMailMessageMetaDataList &metadata);
    void updateMessages(quint64, const QString &filename);
    void updateMessages(quint64, const QMailMessageMetaDataList &metadata);
    void deleteMessages(quint64, const QMailMessageIdList &ids);
    void rollBackUpdates(quint64, const QMailAccountId &mailAccountId);
    void moveToStandardFolder(quint64, const QMailMessageIdList& ids, quint64 standardFolder);
    void moveToFolder(quint64, const QMailMessageIdList& ids, const QMailFolderId& folderId);
    void flagMessages(quint64, const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask);
    void restoreToPreviousFolder(quint64, const QMailMessageKey& key);

    void onlineCreateFolder(quint64, const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    void onlineRenameFolder(quint64, const QMailFolderId &folderId, const QString &name);
    void onlineDeleteFolder(quint64, const QMailFolderId &folderId);

    void cancelTransfer(quint64);

    void searchMessages(quint64, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);
    void searchMessages(quint64, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort);
    void countMessages(quint64, const QMailMessageKey& filter, const QString& bodyText);

    void cancelSearch(quint64);

    void shutdown();

    void listActions();

    void protocolRequest(quint64, const QMailAccountId &accountId, const QString &request, const QVariant &data);


    void actionStarted(QMailActionData);
    void activityChanged(quint64, QMailServiceAction::Activity);
    void connectivityChanged(quint64, QMailServiceAction::Connectivity);
    void statusChanged(quint64, const QMailServiceAction::Status);
    void progressChanged(quint64, uint, uint);

    void retrievalCompleted(quint64);

    void messagesTransmitted(quint64, const QMailMessageIdList&);
    void messagesFailedTransmission(quint64, const QMailMessageIdList&, QMailServiceAction::Status::ErrorCode);
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
    void remainingMessagesCount(quint64, uint);
    void messagesCount(quint64, uint);
    void searchCompleted(quint64);

    void actionsListed(const QMailActionDataList &);

    void protocolResponse(quint64, const QString &response, const QVariant &data);
    void protocolRequestCompleted(quint64);

private:
    QCopAdaptor* adaptor;
};

#endif
