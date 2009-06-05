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

#include "messagestore.h"
#include <qmailaccount.h>
#include <qmailstore.h>

MessageStore::MessageStore(QObject *parent)
    : QObject(parent )
{
    // Create a folder object for each standard folder
    QList<MessageFolder*> folders;
    foreach (QMailFolder::StandardFolder type, 
             QList<QMailFolder::StandardFolder>() << QMailFolder::InboxFolder
                                                  << QMailFolder::SentFolder
                                                  << QMailFolder::DraftsFolder
                                                  << QMailFolder::TrashFolder
                                                  << QMailFolder::OutboxFolder) {
        _mailboxes.append(new MessageFolder(type, this));
    }
}

MessageStore::~MessageStore()
{
}

void MessageStore::openMailboxes()
{
    foreach (MessageFolder* folder, _mailboxes) {
        folder->openMailbox();

        connect(folder, SIGNAL(stringStatus(QString&)), this, SIGNAL(stringStatus(QString&)));
        connect(folder, SIGNAL(externalEdit(QString)), this, SIGNAL(externalEdit(QString)));
        connect(folder, SIGNAL(contentModified()),this, SLOT(folderContentModified()));
    }
}

QMailFolderIdList MessageStore::standardFolders() const
{
    QMailFolderIdList list;

    foreach (const MessageFolder* folder, _mailboxes)
        list.append(folder->id());

    return list;
}

MessageFolder* MessageStore::mailbox(QMailFolder::StandardFolder folder) const
{
    return mailbox(QMailFolderId(folder));
}

MessageFolder* MessageStore::mailbox(const QMailFolderId& mailFolderId) const
{
    if (mailFolderId.isValid())
        foreach (MessageFolder* folder, _mailboxes)
            if (folder->mailFolder().id() == mailFolderId)
                return folder;

    return NULL;

}

MessageFolder* MessageStore::owner(const QMailMessageId& id) const
{
    QMailMessageMetaData metaData(id);
    return mailbox(metaData.parentFolderId());
}

QMailMessageIdList MessageStore::messages(QMailMessage::MessageType messageType, const MessageFolder::SortOrder& order )
{
    return messages(messageFilterKey(messageType), order);
}

QMailMessageIdList MessageStore::messages(quint64 status, 
                                          bool contains,
                                          QMailMessage::MessageType messageType, 
                                          const MessageFolder::SortOrder& order)
{
    return messages(statusFilterKey(status, contains) & messageFilterKey(messageType), order);
}

QMailMessageIdList MessageStore::messagesFromAccount(const QMailAccount& account,
                                                     QMailMessage::MessageType messageType,
                                                     const MessageFolder::SortOrder& order)
{
    return messages(messageFilterKey(messageType, account.id()), order);
}

QMailFolderIdList MessageStore::foldersFromAccount(const QMailAccount& account)
{
    return folders(QMailFolderKey::parentAccountId(account.id()));
}

QMailMessageIdList MessageStore::messages(QMailMessageKey queryKey, const MessageFolder::SortOrder& order)
{
    if (order != MessageFolder::Submission) {
        Qt::SortOrder srtOrder(order == MessageFolder::DescendingDate ? Qt::DescendingOrder : Qt::AscendingOrder);
        QMailMessageSortKey sortKey(QMailMessageSortKey::timeStamp(srtOrder));
        return QMailStore::instance()->queryMessages(queryKey, sortKey);
    } else {
        return QMailStore::instance()->queryMessages(queryKey);
    }
}

QMailFolderIdList MessageStore::folders(QMailFolderKey queryKey)
{
    return QMailStore::instance()->queryFolders(queryKey);
}

uint MessageStore::messageCount( MessageFolder::MailType status, QMailMessage::MessageType type )
{
    return messageCount(statusFilterKey(status) & messageFilterKey(type));
}

uint MessageStore::messageCount( MessageFolder::MailType status, QMailMessage::MessageType type, const QMailAccount& account )
{
    return messageCount(statusFilterKey(status) & messageFilterKey(type, account.id()));
}

uint MessageStore::messageCount(QMailMessageKey queryKey)
{
    return QMailStore::instance()->countMessages(queryKey);
}

QMailMessageKey MessageStore::statusFilterKey( MessageFolder::MailType status )
{
    QMailMessageKey statusKey;

    if (status == MessageFolder::Unfinished) {
        // Currently unhandled!
    } else if (status == MessageFolder::Unread) {
        // Ensure 'read' && 'read-elsewhere' flags are not set
        statusKey = (~QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Includes) &
                     ~QMailMessageKey::status(QMailMessage::ReadElsewhere, QMailDataComparator::Includes));
    } else if (status == MessageFolder::Unsent) {
        // Ensure 'sent' flag is not set
        statusKey = ~QMailMessageKey::status(QMailMessage::Sent, QMailDataComparator::Includes);
    }

    return statusKey;
}

QMailMessageKey MessageStore::statusFilterKey( quint64 status, bool contains )
{
    QMailMessageKey statusKey(QMailMessageKey::status(status, QMailDataComparator::Includes));
    return (contains ? statusKey : ~statusKey);
}

QMailMessageKey MessageStore::messageFilterKey( QMailMessage::MessageType type, const QMailAccountId& accountId, const QMailFolderId& mailboxId, bool subfolders )
{
    QMailMessageKey filter;

    if (type != QMailMessage::AnyType)
        filter = QMailMessageKey::messageType(type, QMailDataComparator::Includes);

    if (accountId.isValid()) {
        filter &= QMailMessageKey::parentAccountId(accountId);

        if (mailboxId.isValid()) {
            QMailMessageKey mailboxKey(QMailMessageKey::parentFolderId(mailboxId)); 
            if (subfolders)
                mailboxKey |= QMailMessageKey::ancestorFolderIds(mailboxId, QMailDataComparator::Includes); 

            filter &= mailboxKey;
        }
    }

    return filter;
}

void MessageStore::folderContentModified()
{
    if (MessageFolder* folder = static_cast<MessageFolder*>(sender()))
        emit contentModified(folder);
}

