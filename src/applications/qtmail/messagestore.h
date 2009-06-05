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

#ifndef MESSAGESTORE_H
#define MESSAGESTORE_H

#include "messagestore.h"
#include "messagefolder.h"
#include <QList>
#include <qmailmessage.h>
#include <qmailmessagekey.h>
#include <qmailfolderkey.h>
#include <QString>

class QMailAccount;

class MessageStore : public QObject
{
    Q_OBJECT

public:
    MessageStore(QObject *parent = 0);
    ~MessageStore();

    void openMailboxes();

    QMailFolderIdList standardFolders() const;

    MessageFolder* mailbox(QMailFolder::StandardFolder folder) const;
    MessageFolder* mailbox(const QMailFolderId& mailFolderId) const;

    MessageFolder* owner(const QMailMessageId& id) const;

    static QMailMessageIdList messages(QMailMessage::MessageType type = QMailMessage::AnyType, 
                                       const MessageFolder::SortOrder& order = MessageFolder::Submission);

    static QMailMessageIdList messages(quint64 status, 
                                       bool contains,
                                       QMailMessage::MessageType type = QMailMessage::AnyType,
                                       const MessageFolder::SortOrder& order = MessageFolder::Submission);

    static QMailMessageIdList messagesFromAccount(const QMailAccount& account, 
                                                  QMailMessage::MessageType type = QMailMessage::AnyType,
                                                  const MessageFolder::SortOrder& order = MessageFolder::Submission);

    static QMailFolderIdList foldersFromAccount(const QMailAccount& account);

    static QMailMessageIdList messages(QMailMessageKey queryKey, const MessageFolder::SortOrder& order);
    static QMailFolderIdList folders(QMailFolderKey queryKey);

    static uint messageCount( MessageFolder::MailType status, QMailMessage::MessageType type = QMailMessage::AnyType );
    static uint messageCount( MessageFolder::MailType status, QMailMessage::MessageType type, const QMailAccount& account );

    static uint messageCount( QMailMessageKey queryKey );

    static QMailMessageKey statusFilterKey(MessageFolder::MailType status);
    static QMailMessageKey statusFilterKey(quint64 status, bool contains);
    static QMailMessageKey messageFilterKey(QMailMessage::MessageType type, 
                                            const QMailAccountId& accountId = QMailAccountId(), 
                                            const QMailFolderId& mailboxId = QMailFolderId(), 
                                            bool subfolders = false);

signals:
    void externalEdit(const QString &);
    void contentModified(const MessageFolder* folder);
    void stringStatus(QString &);

protected slots:
    void folderContentModified();

private:
    QList<MessageFolder*> _mailboxes;
};

#endif
