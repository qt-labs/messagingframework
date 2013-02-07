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

#include "mailmessageclient.h"
#include <qcopadaptor.h>

static bool connectIpc( QObject *sender, const QByteArray& signal,
        QObject *receiver, const QByteArray& member)
{
    return QCopAdaptor::connect(sender,signal,receiver,member);
}

MailMessageClient::MailMessageClient(QObject* parent)
    : QObject(parent),
      adaptor(new QCopAdaptor("QPE/QMailMessageServer",this))
{
    connectIpc(this, SIGNAL(newCountChanged(QMailMessageCountMap)),
                              adaptor, MESSAGE(newCountChanged(QMailMessageCountMap)));
    connectIpc(adaptor, MESSAGE(acknowledgeNewMessages(QMailMessageTypeList)),
                              this, SIGNAL(acknowledgeNewMessages(QMailMessageTypeList)));

    connectIpc(this, SIGNAL(actionStarted(QMailActionData)),
               adaptor, MESSAGE(actionStarted(QMailActionData)));
    connectIpc(this, SIGNAL(activityChanged(quint64, QMailServiceAction::Activity)),
               adaptor, MESSAGE(activityChanged(quint64, QMailServiceAction::Activity)));
    connectIpc(this, SIGNAL(connectivityChanged(quint64, QMailServiceAction::Connectivity)),
               adaptor, MESSAGE(connectivityChanged(quint64, QMailServiceAction::Connectivity)));
    connectIpc(this, SIGNAL(statusChanged(quint64, const QMailServiceAction::Status)),
               adaptor, MESSAGE(statusChanged(quint64, const QMailServiceAction::Status)));
    connectIpc(this, SIGNAL(progressChanged(quint64, uint, uint)),
               adaptor, MESSAGE(progressChanged(quint64, uint, uint)));
    connectIpc(this, SIGNAL(transmissionCompleted(quint64)), 
               adaptor, MESSAGE(transmissionCompleted(quint64)));
    connectIpc(this, SIGNAL(messagesTransmitted(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(messagesTransmitted(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(messagesFailedTransmission(quint64, QMailMessageIdList, QMailServiceAction::Status::ErrorCode)), 
               adaptor, MESSAGE(messagesFailedTransmission(quint64, QMailMessageIdList, QMailServiceAction::Status::ErrorCode)));
    connectIpc(this, SIGNAL(retrievalCompleted(quint64)),
               adaptor, MESSAGE(retrievalCompleted(quint64)));
    connectIpc(this, SIGNAL(messagesDeleted(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(messagesDeleted(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(messagesCopied(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(messagesCopied(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(messagesMoved(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(messagesMoved(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(messagesFlagged(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(messagesFlagged(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(folderCreated(quint64, QMailFolderId)), 
               adaptor, MESSAGE(folderCreated(quint64, QMailFolderId)));
    connectIpc(this, SIGNAL(folderRenamed(quint64, QMailFolderId)), 
               adaptor, MESSAGE(folderRenamed(quint64, QMailFolderId)));
    connectIpc(this, SIGNAL(folderDeleted(quint64, QMailFolderId)), 
               adaptor, MESSAGE(folderDeleted(quint64, QMailFolderId)));
    connectIpc(this, SIGNAL(storageActionCompleted(quint64)), 
               adaptor, MESSAGE(storageActionCompleted(quint64)));
    connectIpc(this, SIGNAL(matchingMessageIds(quint64, QMailMessageIdList)), 
               adaptor, MESSAGE(matchingMessageIds(quint64, QMailMessageIdList)));
    connectIpc(this, SIGNAL(remainingMessagesCount(quint64, uint)), 
               adaptor, MESSAGE(remainingMessagesCount(quint64, uint)));
    connectIpc(this, SIGNAL(messagesCount(quint64, uint)), 
               adaptor, MESSAGE(messagesCount(quint64, uint)));
    connectIpc(this, SIGNAL(searchCompleted(quint64)), 
               adaptor, MESSAGE(searchCompleted(quint64)));
    connectIpc(this, SIGNAL(actionsListed(QMailActionDataList)),
               adaptor, MESSAGE(actionsListed(QMailActionDataList)));
    connectIpc(this, SIGNAL(protocolResponse(quint64, QString, QVariant)),
               adaptor, MESSAGE(protocolResponse(quint64, QString, QVariant)));
    connectIpc(this, SIGNAL(protocolRequestCompleted(quint64)),
               adaptor, MESSAGE(protocolRequestCompleted(quint64)));

    connectIpc(adaptor, MESSAGE(transmitMessages(quint64, QMailAccountId)),
               this, SIGNAL(transmitMessages(quint64, QMailAccountId)));
    connectIpc(adaptor, MESSAGE(transmitMessage(quint64, QMailMessageId)),
               this, SIGNAL(transmitMessage(quint64, QMailMessageId)));
    connectIpc(adaptor, MESSAGE(retrieveFolderList(quint64, QMailAccountId, QMailFolderId, bool)),
               this, SIGNAL(retrieveFolderList(quint64, QMailAccountId, QMailFolderId, bool)));
    connectIpc(adaptor, MESSAGE(retrieveMessageLists(quint64, QMailAccountId, QMailFolderIdList, uint, QMailMessageSortKey)),
               this, SIGNAL(retrieveMessageLists(quint64, QMailAccountId, QMailFolderIdList, uint, QMailMessageSortKey)));
    connectIpc(adaptor, MESSAGE(retrieveMessageList(quint64, QMailAccountId, QMailFolderId, uint, QMailMessageSortKey)),
               this, SIGNAL(retrieveMessageList(quint64, QMailAccountId, QMailFolderId, uint, QMailMessageSortKey)));
    connectIpc(adaptor, MESSAGE(retrieveNewMessages(quint64, QMailAccountId, QMailFolderIdList)),
               this, SIGNAL(retrieveNewMessages(quint64, QMailAccountId, QMailFolderIdList)));
    connectIpc(adaptor, MESSAGE(createStandardFolders(quint64, QMailAccountId)),
               this, SIGNAL(createStandardFolders(quint64, QMailAccountId)));
    connectIpc(adaptor, MESSAGE(retrieveMessages(quint64, QMailMessageIdList, QMailRetrievalAction::RetrievalSpecification)),
               this, SIGNAL(retrieveMessages(quint64, QMailMessageIdList, QMailRetrievalAction::RetrievalSpecification)));
    connectIpc(adaptor, MESSAGE(retrieveMessagePart(quint64, QMailMessagePart::Location)),
               this, SIGNAL(retrieveMessagePart(quint64, QMailMessagePart::Location)));
    connectIpc(adaptor, MESSAGE(retrieveMessageRange(quint64, QMailMessageId, uint)),
               this, SIGNAL(retrieveMessageRange(quint64, QMailMessageId, uint)));
    connectIpc(adaptor, MESSAGE(retrieveMessagePartRange(quint64, QMailMessagePart::Location, uint)),
               this, SIGNAL(retrieveMessagePartRange(quint64, QMailMessagePart::Location, uint)));
    connectIpc(adaptor, MESSAGE(retrieveAll(quint64, QMailAccountId)),
               this, SIGNAL(retrieveAll(quint64, QMailAccountId)));
    connectIpc(adaptor, MESSAGE(exportUpdates(quint64, QMailAccountId)),
               this, SIGNAL(exportUpdates(quint64, QMailAccountId)));
    connectIpc(adaptor, MESSAGE(synchronize(quint64, QMailAccountId)),
               this, SIGNAL(synchronize(quint64, QMailAccountId)));
    connectIpc(adaptor, MESSAGE(onlineDeleteMessages(quint64, QMailMessageIdList, QMailStore::MessageRemovalOption)),
               this, SIGNAL(onlineDeleteMessages(quint64, QMailMessageIdList, QMailStore::MessageRemovalOption)));
    connectIpc(adaptor, MESSAGE(onlineCopyMessages(quint64, QMailMessageIdList, QMailFolderId)),
               this, SIGNAL(onlineCopyMessages(quint64, QMailMessageIdList, QMailFolderId)));
    connectIpc(adaptor, MESSAGE(onlineMoveMessages(quint64, QMailMessageIdList, QMailFolderId)),
               this, SIGNAL(onlineMoveMessages(quint64, QMailMessageIdList, QMailFolderId)));
    connectIpc(adaptor, MESSAGE(onlineFlagMessagesAndMoveToStandardFolder(quint64, QMailMessageIdList, quint64, quint64)),
               this, SIGNAL(onlineFlagMessagesAndMoveToStandardFolder(quint64, QMailMessageIdList, quint64, quint64)));
    connectIpc(adaptor, MESSAGE(addMessages(quint64,QString)),
               this, SIGNAL(addMessages(quint64, QString)));
    connectIpc(adaptor, MESSAGE(addMessages(quint64,QMailMessageMetaDataList)),
               this, SIGNAL(addMessages(quint64, QMailMessageMetaDataList)));
    connectIpc(adaptor, MESSAGE(updateMessages(quint64,QString)),
               this, SIGNAL(updateMessages(quint64, QString)));
    connectIpc(adaptor, MESSAGE(updateMessages(quint64,QMailMessageMetaDataList)),
               this, SIGNAL(updateMessages(quint64, QMailMessageMetaDataList)));
    connectIpc(adaptor, MESSAGE(deleteMessages(quint64, QMailMessageIdList)),
               this, SIGNAL(deleteMessages(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(rollBackUpdates(quint64, QMailAccountId)),
               this, SIGNAL(rollBackUpdates(quint64, QMailAccountId)));
    connectIpc(adaptor, MESSAGE(moveToStandardFolder(quint64, QMailMessageIdList, quint64)),
               this, SIGNAL(moveToStandardFolder(quint64, QMailMessageIdList, quint64)));
    connectIpc(adaptor, MESSAGE(moveToFolder(quint64, QMailMessageIdList, QMailFolderId)),
               this, SIGNAL(moveToFolder(quint64, QMailMessageIdList, QMailFolderId)));
    connectIpc(adaptor, MESSAGE(flagMessages(quint64, QMailMessageIdList, quint64, quint64)),
               this, SIGNAL(flagMessages(quint64, QMailMessageIdList, quint64, quint64)));
    connectIpc(adaptor, MESSAGE(restoreToPreviousFolder(quint64, QMailMessageKey)),
               this, SIGNAL(restoreToPreviousFolder(quint64, QMailMessageKey)));
    connectIpc(adaptor, MESSAGE(onlineCreateFolder(quint64, QString, QMailAccountId, QMailFolderId)),
               this, SIGNAL(onlineCreateFolder(quint64,QString,QMailAccountId,QMailFolderId)));
    connectIpc(adaptor, MESSAGE(onlineRenameFolder(quint64, QMailFolderId, QString)),
               this, SIGNAL(onlineRenameFolder(quint64,QMailFolderId,QString)));
    connectIpc(adaptor, MESSAGE(onlineDeleteFolder(quint64, QMailFolderId)),
               this, SIGNAL(onlineDeleteFolder(quint64,QMailFolderId)));
    connectIpc(adaptor, MESSAGE(cancelTransfer(quint64)),
               this, SIGNAL(cancelTransfer(quint64)));
    connectIpc(adaptor, MESSAGE(cancelSearch(quint64)),
               this, SIGNAL(cancelSearch(quint64)));
    connectIpc(adaptor, MESSAGE(shutdown()),
               this, SIGNAL(shutdown()));
    connectIpc(adaptor, MESSAGE(listActions()),
               this, SIGNAL(listActions()));
    connectIpc(adaptor, MESSAGE(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, QMailMessageSortKey)),
               this, SIGNAL(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, QMailMessageSortKey)));
    connectIpc(adaptor, MESSAGE(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, quint64, QMailMessageSortKey)),
               this, SIGNAL(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, quint64, QMailMessageSortKey)));
    connectIpc(adaptor, MESSAGE(countMessages(quint64, QMailMessageKey, QString)),
               this, SIGNAL(countMessages(quint64, QMailMessageKey, QString)));
    connectIpc(adaptor, MESSAGE(protocolRequest(quint64, QMailAccountId, QString, QVariant)),
               this, SIGNAL(protocolRequest(quint64, QMailAccountId, QString, QVariant)));
}

MailMessageClient::~MailMessageClient()
{
}

