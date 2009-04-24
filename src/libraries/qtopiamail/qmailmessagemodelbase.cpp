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
#include "qmailstore.h"
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
            return qApp->translate("QMailMessageModelBase", "Draft message");
        }
    }
}

}


QModelIndex QMailMessageModelImplementation::updateIndex(const QModelIndex &idx) const
{
    return idx;
}

QList<QMailMessageModelImplementation::LocationSequence> QMailMessageModelImplementation::indicesToLocationSequence(const QList<int> &indices)
{
    QList<LocationSequence> result;

    QList<int>::const_iterator it = indices.begin(), end = indices.end();
    for (; it != end; ++it) {
        LocationSequence loc;
        loc.first = QModelIndex();
        loc.second.first = *it;
        loc.second.second = *it;
        
        // See if the following indices form a sequence
        QList<int>::const_iterator next = it + 1;
        while (next != end) {
            if ((*next == loc.second.second) || (*next == (loc.second.second + 1))) {
                // This ID is part of the same sequence
                loc.second.second += 1;
                ++it;
                ++next;
            } else {
                next = end;
            }
        }

        result.append(loc);
    }

    return result;
}

QList<QMailMessageModelImplementation::LocationSequence> QMailMessageModelImplementation::indicesToLocationSequence(const QList<QPair<QModelIndex, int> > &indices)
{
    QList<LocationSequence> result;

    QList<QPair<QModelIndex, int> >::const_iterator it = indices.begin(), end = indices.end();
    for (; it != end; ++it) {
        LocationSequence loc;
        loc.first = (*it).first;
        loc.second.first = (*it).second;
        loc.second.second = (*it).second;
        
        // See if the following indices form a sequence
        QList<QPair<QModelIndex, int> >::const_iterator next = it + 1;
        while (next != end) {
            if (((*next).first == loc.first) && 
                (((*next).second == loc.second.second) || ((*next).second == (loc.second.second + 1)))) {
                // This ID is part of the same sequence
                loc.second.second += 1;
                ++it;
                ++next;
            } else {
                next = end;
            }
        }

        result.append(loc);
    }

    return result;
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

/*!
    Constructs a QMailMessageModelBase with the parent \a parent.
*/
QMailMessageModelBase::QMailMessageModelBase(QObject* parent)
    : QAbstractItemModel(parent)
{
    connect(QMailStore::instance(), SIGNAL(messagesAdded(QMailMessageIdList)), this, SLOT(messagesAdded(QMailMessageIdList)));
    connect(QMailStore::instance(), SIGNAL(messagesRemoved(QMailMessageIdList)), this, SLOT(messagesRemoved(QMailMessageIdList)));
    connect(QMailStore::instance(), SIGNAL(messagesUpdated(QMailMessageIdList)), this, SLOT(messagesUpdated(QMailMessageIdList)));
}

/*! \internal */
QMailMessageModelBase::~QMailMessageModelBase()
{
}

/*! \reimp */
int QMailMessageModelBase::rowCount(const QModelIndex& index) const
{
    return impl()->rowCount(index);
}

/*! \reimp */
int QMailMessageModelBase::columnCount(const QModelIndex& index) const
{
    return impl()->columnCount(index);
}

/*!
    Returns true if the model contains no messages.
*/
bool QMailMessageModelBase::isEmpty() const
{
    return impl()->isEmpty();
}

/*! \reimp */
QVariant QMailMessageModelBase::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QMailMessageId id = idFromIndex(index);
    if (!id.isValid())
        return QVariant();

    // Some items can be processed without loading the message data
    switch(role)
    {
    case MessageIdRole:
        return id;
        break;

    case Qt::CheckStateRole:
        return impl()->checkState(index);
        break;

    default:
        break;
    }

    // Otherwise, load the message data
    return data(QMailMessageMetaData(id), role);
}

/*! \internal */
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

/*! \reimp */
bool QMailMessageModelBase::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid()) {
        // The only role we allow to be changed is the check state
        if (role == Qt::CheckStateRole || role == Qt::EditRole) {
            impl()->setCheckState(index, static_cast<Qt::CheckState>(value.toInt()));
            emit dataChanged(index, index);
            return true;
        }
    }

    return false;
}

/*!
    Returns the QMailMessageKey used to populate the contents of this model.
*/
QMailMessageKey QMailMessageModelBase::key() const
{
    return impl()->key(); 
}

/*!
    Sets the QMailMessageKey used to populate the contents of the model to \a key.
    If the key is empty, the model is populated with all the messages from the 
    database.
*/
void QMailMessageModelBase::setKey(const QMailMessageKey& key) 
{
    impl()->setKey(key);
    fullRefresh(true);
}

/*!
    Returns the QMailMessageSortKey used to sort the contents of the model.
*/
QMailMessageSortKey QMailMessageModelBase::sortKey() const
{
   return impl()->sortKey();
}

/*!
    Sets the QMailMessageSortKey used to sort the contents of the model to \a sortKey.
    If the sort key is invalid, no sorting is applied to the model contents and messages
    are displayed in the order in which they were added into the database.
*/
void QMailMessageModelBase::setSortKey(const QMailMessageSortKey& sortKey) 
{
    // We need a sort key defined, to preserve the ordering in DB records for addition/removal events
    impl()->setSortKey(sortKey.isEmpty() ? QMailMessageSortKey::id() : sortKey);
    fullRefresh(true);
}

/*! \internal */
void QMailMessageModelBase::messagesAdded(const QMailMessageIdList& ids)
{
    QList<QMailMessageModelImplementation::LocationSequence> locations;
    QMailMessageIdList insertIds;

    // Find where these messages should be added
    if (!impl()->additionLocations(ids, &locations, &insertIds)) {
        // We need to refresh the entire content
        fullRefresh(false);
    } else if (!locations.isEmpty()) {
        QMailMessageIdList::const_iterator it = insertIds.begin();

        foreach (const QMailMessageModelImplementation::LocationSequence &seq, locations) {
            const QPair<int, int> rows(seq.second);

            QModelIndex parentIndex(impl()->updateIndex(seq.first));

            // Insert the item(s) at this location
            beginInsertRows(parentIndex, rows.first, rows.second);
            for (int i = rows.first; i <= rows.second; ++i) {
                impl()->insertItemAt(i, parentIndex, *it);
                ++it;
            }
            endInsertRows();
        }
    }
}

/*! \internal */
void QMailMessageModelBase::messagesUpdated(const QMailMessageIdList& ids)
{
    QList<QMailMessageModelImplementation::LocationSequence> insertions;
    QList<QMailMessageModelImplementation::LocationSequence> removals;
    QList<QMailMessageModelImplementation::LocationSequence> updates;
    QMailMessageIdList insertIds;

    // Find where these messages should be added/removed/updated
    if (!impl()->updateLocations(ids, &insertions, &removals, &updates, &insertIds)) {
        // We need to refresh the entire content
        fullRefresh(false);
    } else {
        // Remove any items that are no longer present
        if (!removals.isEmpty()) {
            // Remove the items in reverse order
            for (int n = removals.count(); n > 0; --n) {
                const QMailMessageModelImplementation::LocationSequence &seq(removals.at(n - 1));
                const QPair<int, int> rows(seq.second);

                QModelIndex parentIndex(impl()->updateIndex(seq.first));

                beginRemoveRows(parentIndex, rows.first, rows.second);
                for (int i = rows.second; i >= rows.first; --i) {
                    impl()->removeItemAt(i, parentIndex);
                }
                endRemoveRows();
            }
        }

        // Insert any new items
        QMailMessageIdList::const_iterator it = insertIds.begin();

        foreach (const QMailMessageModelImplementation::LocationSequence &seq, insertions) {
            const QPair<int, int> rows(seq.second);

            QModelIndex parentIndex(impl()->updateIndex(seq.first));

            // Insert the item(s) at this location
            beginInsertRows(parentIndex, rows.first, rows.second);
            for (int i = rows.first; i <= rows.second; ++i) {
                impl()->insertItemAt(i, parentIndex, *it);
                ++it;
            }
            endInsertRows();
        }

        // Update any items still at the same location
        foreach (const QMailMessageModelImplementation::LocationSequence &seq, updates) {
            // Ignore the rows, these indexes are to items rather than parents
            emit dataChanged(seq.first, seq.first);
        }
    }
}

/*! \internal */
void QMailMessageModelBase::messagesRemoved(const QMailMessageIdList& ids)
{
    QList<QMailMessageModelImplementation::LocationSequence> locations;

    // Find where these messages should be removed from
    if (!impl()->removalLocations(ids, &locations)) {
        // We need to refresh the entire content
        fullRefresh(false);
    } else if (!locations.isEmpty()) {
        // Remove the items in reverse order
        for (int n = locations.count(); n > 0; --n) {
            const QMailMessageModelImplementation::LocationSequence &seq(locations.at(n - 1));
            const QPair<int, int> rows(seq.second);

            QModelIndex parentIndex(impl()->updateIndex(seq.first));

            beginRemoveRows(parentIndex, rows.first, rows.second);
            for (int i = rows.second; i >= rows.first; --i) {
                impl()->removeItemAt(i, parentIndex);
            }
            endRemoveRows();
        }
    }
}

/*!
    Returns the QMailMessageId of the message represented by the QModelIndex \a index.
    If the index is not valid an invalid QMailMessageId is returned.
*/
QMailMessageId QMailMessageModelBase::idFromIndex(const QModelIndex& index) const
{
    return impl()->idFromIndex(index);
}

/*!
    Returns the QModelIndex that represents the message with QMailMessageId \a id.
    If the id is not contained in this model, an invalid QModelIndex is returned.
*/
QModelIndex QMailMessageModelBase::indexFromId(const QMailMessageId& id) const
{
    return impl()->indexFromId(id);
}

/*!
    Returns true if the model has been set to ignore updates emitted by 
    the mail store; otherwise returns false.
*/
bool QMailMessageModelBase::ignoreMailStoreUpdates() const
{
    return impl()->ignoreMailStoreUpdates();
}

/*!
    Sets whether or not mail store updates are ignored to \a ignore.

    If ignoring updates is set to true, the model will ignore updates reported 
    by the mail store.  If set to false, the model will automatically synchronize 
    its content in reaction to updates reported by the mail store.

    If updates are ignored, signals such as rowInserted and dataChanged will not 
    be emitted; instead, the modelReset signal will be emitted when the model is
    later changed to stop ignoring mail store updates, and detailed change 
    information will not be accessible.
*/
void QMailMessageModelBase::setIgnoreMailStoreUpdates(bool ignore)
{
    if (impl()->setIgnoreMailStoreUpdates(ignore))
        fullRefresh(false);
}

/*! \internal */
void QMailMessageModelBase::fullRefresh(bool changed) 
{
    impl()->reset();
    reset();

    if (changed)
        emit modelChanged();
}

