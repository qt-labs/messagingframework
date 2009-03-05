/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
