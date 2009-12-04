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

#include "messagemodel.h"
#include "messagedelegate.h"
#include <QIcon>
#include <QMailMessageId>
#include <QMailMessageMetaData>
#include <QMailMessageKey>
#include <QMailMessageSortKey>
#include <QMailStore>
#include <QPhoneNumber>
#include <QStandardItem>
#include <QTimeString>
#include <QtopiaApplication>


class MessageItem : public QStandardItem
{
public:
    explicit MessageItem(const QMailMessageId& id);
    virtual ~MessageItem();

    QMailMessageId messageId() const;

private:
    QMailMessageId id;
};

MessageItem::MessageItem(const QMailMessageId& id)
    : QStandardItem(), id(id)
{
    static QIcon sentMessageIcon(":icon/qtmail/sendmail");
    static QIcon receivedMessageIcon(":icon/qtmail/getmail");
    static QIcon smsIcon(":icon/txt");
    static QIcon mmsIcon(":icon/multimedia");
    static QIcon emailIcon(":icon/email");
    static QIcon instantIcon(":icon/im");

    // Load the meta data for this message
    QMailMessageMetaData message(id);

    // Determine the properties we want to display
    QIcon* messageIcon = &smsIcon;
    if (message.messageType() == QMailMessage::Mms)
        messageIcon = &mmsIcon;
    if (message.messageType() == QMailMessage::Email)
        messageIcon = &emailIcon;
    if (message.messageType() == QMailMessage::Instant)
        messageIcon = &instantIcon;

    bool sent(message.status() & QMailMessage::Outgoing);

    QDateTime messageTime(message.date().toLocalTime());

    QString action(qApp->translate("MessageViewer", sent ? "Sent" : "Received"));
    QString date(QTimeString::localMD(messageTime.date()));
    QString time(QTimeString::localHM(messageTime.time(), QTimeString::Short));
    QString sublabel(QString("%1 %2 %3").arg(action).arg(date).arg(time));

    // Configure this item
    setIcon(sent ? sentMessageIcon : receivedMessageIcon);
    setText(message.subject());
    setData(sublabel, MessageDelegate::SubLabelRole);
    setData(*messageIcon, MessageDelegate::SecondaryDecorationRole);
}

MessageItem::~MessageItem()
{
}

QMailMessageId MessageItem::messageId() const
{
    return id;
}


MessageModel::MessageModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

MessageModel::~MessageModel()
{
}

void MessageModel::setContact(const QContact& contact)
{
    clear();

    if (contact.phoneNumbers().isEmpty() && contact.emailList().isEmpty()) {
        // Nothing to match for this contact
        return;
    }

    // Locate messages whose sender is this contact
    QMailMessageKey msgsFrom;

    // Locate messages whose recipients list contains this contact
    QMailMessageKey msgsTo;

    // Match on any of contact's phone numbers
    foreach(const QString& number, contact.phoneNumbers().values()) {
        msgsFrom |= QMailMessageKey::sender(number);
        msgsTo |= QMailMessageKey::recipients(number, QMailDataComparator::Includes);
    }

    // Match on any of contact's email addresses
    foreach(const QString& address, contact.emailList()) {
        msgsFrom |= QMailMessageKey::sender(address);
        msgsTo |= QMailMessageKey::recipients(address, QMailDataComparator::Includes);
    }

    // Sort messages by timestamp, newest to oldest
    QMailMessageSortKey sort(QMailMessageSortKey::timeStamp(Qt::DescendingOrder));

    // Fetch the messages matching either of our queries, and return them sorted
    QMailMessageIdList matches(QMailStore::instance()->queryMessages(msgsFrom | msgsTo, sort));

    // Add each returned message to our data model
    foreach (const QMailMessageId& id, matches)
        appendRow(new MessageItem(id));
}

bool MessageModel::isEmpty() const
{
    return (rowCount() == 0);
}

QMailMessageId MessageModel::messageId(const QModelIndex& index)
{
    if (index.isValid())
        if (MessageItem* item = static_cast<MessageItem*>(itemFromIndex(index)))
            return item->messageId();

    return QMailMessageId();
}

