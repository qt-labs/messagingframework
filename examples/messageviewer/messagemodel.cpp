/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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

