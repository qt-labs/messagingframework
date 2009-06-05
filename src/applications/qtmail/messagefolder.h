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

#ifndef MESSAGEFOLDER_H
#define MESSAGEFOLDER_H

#include <qmailfolder.h>
#include <qmailmessage.h>
#include <qmailmessagekey.h>
#include <QString>

class QMailAccount;
class SystemFolder;

class MessageFolder : public QObject
{
    Q_OBJECT

public:
    enum MailType { All, Unread, Unsent, Unfinished };
    enum SortOrder { Submission, AscendingDate, DescendingDate };

    MessageFolder(const QMailFolder::StandardFolder& folder, QObject* parent = 0);
    MessageFolder(const QMailFolderId& id, QObject *parent = 0);
    ~MessageFolder();

    QString mailbox() const;
    QMailFolder mailFolder() const;

    QMailFolderId id() const;

    bool insertMessage(QMailMessage &message);
    bool insertMessage(QMailMessageMetaData &message);

    bool moveMessage(const QMailMessageId &id);
    bool moveMessages(const QMailMessageIdList &ids);

    bool copyMessage(const QMailMessageId& id);
    bool copyMessages(const QMailMessageIdList& ids);

    bool deleteMessage(const QMailMessageId& id);
    bool deleteMessages(const QMailMessageIdList& ids);

    bool contains(const QMailMessageId& id) const;

    QMailMessageIdList messages(QMailMessage::MessageType type = QMailMessage::AnyType, 
                                const SortOrder& order = Submission ) const;

    QMailMessageIdList messages(quint64 status, 
                                bool contains,
                                QMailMessage::MessageType type = QMailMessage::AnyType,
                                const SortOrder& order = Submission ) const;

    QMailMessageIdList messagesFromAccount(const QMailAccount& account, 
                                           QMailMessage::MessageType type = QMailMessage::AnyType,
                                           const SortOrder& order = Submission ) const;

    uint messageCount( MailType status, QMailMessage::MessageType type = QMailMessage::AnyType ) const;
    uint messageCount( MailType status, QMailMessage::MessageType type, const QMailAccount& account ) const;

signals:
    void externalEdit(const QString &);
    void contentModified();
    void stringStatus(QString &);

protected slots:
    void externalChange();
    void folderContentsModified(const QMailFolderIdList&);

private:
    QMailMessageIdList messages(QMailMessageKey queryKey, const SortOrder& order) const;
    uint messageCount(QMailMessageKey queryKey) const;

    friend class MessageStore;
    void openMailbox();

    QMailFolder mFolder;
    QMailMessageKey mParentFolderKey;
};

#endif
