/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailmessagemodelbase.h"
#include "qmailmessage.h"
#include <QCoreApplication>
#include <QIcon>

namespace {

QString messageAddressText(const QMailMessageMetaData& m, bool incoming) 
{
    //message sender or recipients
    if ( incoming ) {
        QMailAddress fromAddress(m.from());
        return fromAddress.toString();
    } else {
        QMailAddressList toAddressList(m.to());
        if (!toAddressList.isEmpty()) {
            QMailAddress firstRecipient(toAddressList.first());
            QString text = firstRecipient.toString();

            bool multipleRecipients(toAddressList.count() > 1);
            if (multipleRecipients)
                text += ", ...";

            return text;
        } else  {
            return qApp->translate("QMailMessageModelBase", "Draft Message");
        }
    }
}

}

/*!
    \enum QMailMessageModelBase::Roles

    Represents common display roles of a message. These roles are used to display common message elements 
    in a view and its attached delegates.

    \value MessageAddressTextRole 
        The address text of a message. This a can represent a name if the address is tied to a contact in the addressbook and 
        represents either the incoming or outgoing address depending on the message direction.
    \value MessageSubjectTextRole  
        The subject of a message. For-non email messages this may represent the body text of a message.
    \value MessageFilterTextRole 
        The MessageAddressTextRole concatenated with the MessageSubjectTextRole. This can be used by filtering classes to filter
        messages based on the text of these commonly displayed roles. 
    \value MessageTimeStampTextRole
        The timestamp of a message. "Recieved" or "Sent" is prepended to the timestamp string depending on the message direction.
    \value MessageTypeIconRole
        An Icon representing the type of the message.
    \value MessageStatusIconRole
        An Icon representing the status of the message. e.g Read, Unread, Downloaded
    \value MessageDirectionIconRole
        An Icon representing the incoming or outgoing direction of a message.
    \value MessagePresenceIconRole
        An Icon representing the presence status of the contact associated with the MessageAddressTextRole.
    \value MessageBodyTextRole  
        The body of a message represented as text.
    \value MessageIdRole
        The QMailMessageId value identifying the message.
*/

QVariant QMailMessageModelBase::data(const QMailMessageMetaData &message, int role) const
{
    static QIcon outgoingIcon(":icon/qtmail/sendmail");
    static QIcon incomingIcon(":icon/qtmail/getmail");

    static QIcon readIcon(":icon/qtmail/flag_normal");
    static QIcon unreadIcon(":icon/qtmail/flag_unread");
    static QIcon toGetIcon(":icon/qtmail/flag_toget");
    static QIcon toSendIcon(":icon/qtmail/flag_tosend");
    static QIcon unfinishedIcon(":icon/qtmail/flag_unfinished");
    static QIcon removedIcon(":icon/qtmail/flag_removed");

    static QIcon noPresenceIcon(":icon/presence-none");
    static QIcon offlineIcon(":icon/presence-offline");
    static QIcon awayIcon(":icon/presence-away");
    static QIcon busyIcon(":icon/presence-busy");
    static QIcon onlineIcon(":icon/presence-online");

    static QIcon messageIcon(":icon/txt");
    static QIcon mmsIcon(":icon/multimedia");
    static QIcon emailIcon(":icon/email");
    static QIcon instantMessageIcon(":icon/im");

    bool sent(message.status() & QMailMessage::Sent);
    bool incoming(message.status() & QMailMessage::Incoming);

    switch(role)
    {
        case Qt::DisplayRole:
        case MessageAddressTextRole:
            return messageAddressText(message,incoming);
        break;

        case MessageTimeStampTextRole:
        {
            QString action;
            if (incoming) {
                action = qApp->translate("QMailMessageModelBase", "Received");
            } else {
                if (sent) {
                    action = qApp->translate("QMailMessageModelBase", "Sent");
                } else {
                    action = qApp->translate("QMailMessageModelBase", "Last edited");
                }
            }

            QDateTime messageTime(message.date().toLocalTime());
            QString date(messageTime.date().toString("dd MMM"));
            QString time(messageTime.time().toString("h:mm"));
            QString sublabel(QString("%1 %2 %3").arg(action).arg(date).arg(time));
            return sublabel;
        }
        break;

        case MessageSubjectTextRole:
            return message.subject();
        break;

        case MessageFilterTextRole:
            return messageAddressText(message,incoming) + " " + message.subject();
        break;

        case Qt::DecorationRole:
        case MessageTypeIconRole:
        {
            QIcon icon = messageIcon;
            if (message.messageType() == QMailMessage::Mms) {
                icon = mmsIcon;
            } else if (message.messageType() == QMailMessage::Email) {
                icon = emailIcon;
            } else if (message.messageType() == QMailMessage::Instant) {
                icon = instantMessageIcon;
            }
            return icon;

        }
        break;

        case MessageDirectionIconRole:
        {
            QIcon mainIcon = incoming ? incomingIcon : outgoingIcon;
            return mainIcon;
        }
        break;

        case MessageStatusIconRole:
        {
            if (incoming){ 
                quint64 status = message.status();
                if ( status & QMailMessage::Removed ) {
                    return removedIcon;
                } else if ( status & QMailMessage::PartialContentAvailable ) {
                    if ( status & QMailMessage::Read || status & QMailMessage::ReadElsewhere ) {
                        return readIcon;
                    } else {
                        return unreadIcon;
                    }
                } else {
                    return toGetIcon;
                }
            } else {
                if (sent) {
                    return readIcon;
                } else if ( message.to().isEmpty() ) {
                    // Not strictly true - there could be CC or BCC addressees
                    return unfinishedIcon;
                } else {
                    return toSendIcon;
                }
            }
        }
        break;

        case MessagePresenceIconRole:
        {
            QIcon icon = noPresenceIcon;
            return icon;
        }
        break;

        case MessageBodyTextRole:
        {
            if ((message.messageType() == QMailMessage::Instant) && !message.subject().isEmpty()) {
                // For IMs that contain only text, the body is replicated in the subject
                return message.subject();
            } else {
                QMailMessage fullMessage(message.id());

                // Some items require the entire message data
                if (fullMessage.hasBody())
                    return fullMessage.body().data();

                return QString();
            }
        }
        break;
    }

    return QVariant();
}

