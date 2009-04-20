/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailmessagelistmodel.h"
#include "qmailstore.h"
#include "qmailnamespace.h"
#include <QIcon>
#include <QDebug>
#include <QCache>
#include <QtAlgorithms>

static const int nameCacheSize = 50;
static const int fullRefreshCutoff = 10;

class QMailMessageListModelPrivate
{
public:
    class Item : private QPair<QMailMessageId, bool>
    {
    public:
        explicit Item(const QMailMessageId& id, bool f = false) : QPair<QMailMessageId, bool>(id, f) {}

        QMailMessageId id() const { return first; }

        bool isChecked() const { return second; }
        void setChecked(bool f) { second = f; }

        // Two instances of the same QMailMessageId are the same Item, regardless of the checked state
        bool operator== (const Item& other) { return (first == other.first); }
    };

    typedef QPair<QModelIndex, QPair<int, int> > LocationSequence;

    QMailMessageListModelPrivate(QMailMessageListModel& model,
                                 const QMailMessageKey& key,
                                 const QMailMessageSortKey& sortKey,
                                 bool sychronizeEnabled);
    ~QMailMessageListModelPrivate();

    const QList<Item>& items() const;

    int indexOf(const QMailMessageId& id) const;

    QString messageAddressText(const QMailMessageMetaData& m, bool incoming);

    void invalidateCache();

    bool additionLocations(const QMailMessageIdList &ids,
                           QList<LocationSequence> *locations, 
                           QMailMessageIdList *insertIds) const;

    bool updateLocations(const QMailMessageIdList &ids, 
                         QList<LocationSequence> *additions, 
                         QList<LocationSequence> *deletions, 
                         QList<LocationSequence> *updates,
                         QMailMessageIdList *insertIds) const;

    bool removalLocations(const QMailMessageIdList &ids, 
                          QList<LocationSequence> *locations) const;

    void insertItemAt(int row, const QModelIndex &parentIndex, const QMailMessageId &id);
    void removeItemAt(int row, const QModelIndex &parentIndex);

public:
    QList<QMailMessageListModelPrivate::LocationSequence> indicesToLocationSequence(const QList<int> &indices) const;

    QMailMessageListModel &model;
    QMailMessageKey key;
    QMailMessageSortKey sortKey;
    bool ignoreUpdates;
    mutable QList<Item> itemList;
    mutable QMap<QMailMessageId, int> itemIndex;
    mutable QMailMessageIdList currentIds;
    mutable bool init;
    mutable bool needSynchronize;
    QCache<QString,QString> nameCache;
};


QMailMessageListModelPrivate::QMailMessageListModelPrivate(QMailMessageListModel& model,
                                                           const QMailMessageKey& key,
                                                           const QMailMessageSortKey& sortKey,
                                                           bool ignoreUpdates)
:
    model(model),
    key(key),
    sortKey(sortKey),
    ignoreUpdates(ignoreUpdates),
    init(false),
    needSynchronize(true),
    nameCache(nameCacheSize)
{
}

QMailMessageListModelPrivate::~QMailMessageListModelPrivate()
{
}

const QList<QMailMessageListModelPrivate::Item>& QMailMessageListModelPrivate::items() const
{
    if (!init) {
        itemList.clear();
        itemIndex.clear();

        int index = 0;
        currentIds = QMailStore::instance()->queryMessages(key, sortKey);
        foreach (const QMailMessageId &id, currentIds) {
            itemList.append(QMailMessageListModelPrivate::Item(id, false));
            itemIndex.insert(id, index);
            ++index;
        }

        init = true;
        needSynchronize = false;
    }

    return itemList;
}

int QMailMessageListModelPrivate::indexOf(const QMailMessageId& id) const
{
    QMap<QMailMessageId, int>::const_iterator it = itemIndex.find(id);
    if (it != itemIndex.end()) {
        return it.value();
    }

    return -1;
}

QString QMailMessageListModelPrivate::messageAddressText(const QMailMessageMetaData& m, bool incoming) 
{
    //message sender or recipients
    if ( incoming ) 
    {
        QMailAddress fromAddress(m.from());
        return fromAddress.toString();

    }
    else 
    {
        QMailAddressList toAddressList(m.to());
        if (!toAddressList.isEmpty())
        {
            QMailAddress firstRecipient(toAddressList.first());
            QString text = firstRecipient.toString();
            bool multipleRecipients(toAddressList.count() > 1);
            if(multipleRecipients)
                text += ", ...";
            return text;
        }
        else 
            return QObject::tr("Draft Message");
    }
}

void QMailMessageListModelPrivate::invalidateCache()
{
    nameCache.clear();
}

bool QMailMessageListModelPrivate::additionLocations(const QMailMessageIdList &ids,
                                                     QList<LocationSequence> *locations, 
                                                     QMailMessageIdList *insertIds) const
{
    // Are any of these messages members of our display set?
    // Note - we must only consider messages in the set given by (those we currently know +
    // those we have now been informed of) because the database content may have changed between
    // when this event was recorded and when we're processing the signal.
    
    QMailMessageKey idKey(QMailMessageKey::id(currentIds + ids));
    QMailMessageIdList newIds(QMailStore::instance()->queryMessages(key & idKey, sortKey));

    int additionCount = newIds.count() - itemList.count();
    if (additionCount <= 0) {
        // Nothing has been added
        return true;
    }

    // Find the locations for these messages by comparing to the existing list
    QList<QMailMessageId>::const_iterator nit = newIds.begin(), nend = newIds.end();
    QList<Item>::const_iterator iit = itemList.begin(), iend = itemList.end();

    QList<int> insertIndices;
    QMap<int, QMailMessageId> indexId;
    for (int index = 0; nit != nend; ++nit, ++index) {
        const QMailMessageId &id(*nit);

        if ((iit == iend) || ((*iit).id() != id)) {
            // We need to insert this item here
            insertIndices.append(index);
            indexId.insert(index, id);
        } else {
            ++iit;
        }
    }

    Q_ASSERT(insertIndices.count() == additionCount);

    *locations = indicesToLocationSequence(insertIndices);
    foreach (int index, insertIndices) {
        insertIds->append(indexId[index]);
    }
    return true;
}

bool QMailMessageListModelPrivate::updateLocations(const QMailMessageIdList &ids, 
                                                   QList<LocationSequence> *additions, 
                                                   QList<LocationSequence> *deletions, 
                                                   QList<LocationSequence> *updates,
                                                   QMailMessageIdList *insertIds) const
{
    QList<int> insertIndices;
    QList<int> removeIndices;
    QList<int> updateIndices;

    // Find the updated positions for our messages
    QMailMessageKey idKey(QMailMessageKey::id((currentIds.toSet() + ids.toSet()).toList()));
    QMailMessageIdList newIds(QMailStore::instance()->queryMessages(key & idKey, sortKey));
    QMap<QMailMessageId, int> newPositions;

    int index = 0;
    foreach (const QMailMessageId &id, newIds) {
        newPositions.insert(id, index);
        ++index;
    }

    int delta = (newIds.count() - itemList.count());

    QMap<int, QMailMessageId> indexId;
    foreach (const QMailMessageId &id, ids) {
        int newIndex = -1;
        QMap<QMailMessageId, int>::const_iterator it = newPositions.find(id);
        if (it != newPositions.end()) {
            newIndex = it.value();
        }

        int oldIndex(indexOf(id));
        if (oldIndex == -1) {
            // This message was not previously in our set - add it
            if (newIndex != -1) {
                insertIndices.append(newIndex);
                indexId.insert(newIndex, id);
            }
        } else {
            // We already had this message
            if (newIndex == -1) {
                removeIndices.append(oldIndex);
            } else {
                if (newIndex == oldIndex) {
                    // This message is updated but has not changed position
                    updateIndices.append(newIndex);
                } else {
                    // The item has changed position - delete and re-add
                    removeIndices.append(oldIndex);
                    insertIndices.append(newIndex);
                    indexId.insert(newIndex, id);
                }
            }
        }
    }

    Q_ASSERT((insertIndices.count() - removeIndices.count()) == delta);

    // Sort the lists to yield ascending order
    qSort(insertIndices);
    qSort(removeIndices);
    qSort(updateIndices);

    *additions = indicesToLocationSequence(insertIndices);
    *deletions = indicesToLocationSequence(removeIndices);
    *updates = indicesToLocationSequence(updateIndices);

    foreach (int index, insertIndices) {
        insertIds->append(indexId[index]);
    }
    return true;
}

bool QMailMessageListModelPrivate::removalLocations(const QMailMessageIdList &ids, QList<LocationSequence> *locations) const
{
    QList<int> removeIndices;
    foreach (const QMailMessageId &id, ids) {
        int index(indexOf(id));
        if (index != -1) {
            removeIndices.append(index);
        }
    }
    
    // Sort the indices to yield ascending order (they must be deleted in descending order!)
    qSort(removeIndices);

    *locations = indicesToLocationSequence(removeIndices);
    return true;
}

void QMailMessageListModelPrivate::insertItemAt(int row, const QModelIndex &parentIndex, const QMailMessageId &id)
{
    itemList.insert(row, QMailMessageListModelPrivate::Item(id));
    itemIndex.insert(id, row);
    currentIds.append(id);

    // Adjust the indices for the items that have been moved
    QList<Item>::iterator it = itemList.begin() + (row + 1), end = itemList.end();
    for ( ; it != end; ++it) {
        itemIndex[(*it).id()] += 1;
    }

    Q_UNUSED(parentIndex)
}

void QMailMessageListModelPrivate::removeItemAt(int row, const QModelIndex &parentIndex)
{
    QMailMessageId id(itemList.at(row).id());
    itemIndex.remove(id);
    itemList.removeAt(row);
    currentIds.append(id);

    // Adjust the indices for the items that have been moved
    QList<Item>::iterator it = itemList.begin() + row, end = itemList.end();
    for ( ; it != end; ++it) {
        itemIndex[(*it).id()] -= 1;
    }

    Q_UNUSED(parentIndex)
}

QList<QMailMessageListModelPrivate::LocationSequence> QMailMessageListModelPrivate::indicesToLocationSequence(const QList<int> &indices) const
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
            if (*next == *(next - 1) + 1) {
                // This ID is part of the same sequence
                loc.second.second = *next;
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
  \class QMailMessageListModel 
    \inpublicgroup QtMessagingModule

  \preliminary
  \ingroup messaginglibrary 
  \brief The QMailMessageListModel class provides access to a list of stored messages. 

  The QMailMessageListModel presents a list of all the messages currently stored in
  the message store. By using the setKey() and sortKey() functions it is possible to have the model
  represent specific user filtered subsets of messages sorted in a particular order.

  The QMailMessageListModel is a descendant of QAbstractListModel, so it is suitable for use with
  the Qt View classes such as QListView to visually represent lists of messages. 
 
  The model listens for changes reported by the QMailStore, and automatically synchronizes
  its content with that of the store.  This behaviour can be optionally or temporarily disabled 
  by calling the setIgnoreMailStoreUpdates() function.

  Messages can be extracted from the view with the idFromIndex() function and the resultant id can be 
  used to load a message from the store. 

  For filters or sorting not provided by the QMailMessageListModel it is recommended that
  QSortFilterProxyModel is used to wrap the model to provide custom sorting and filtering. 

  \sa QMailMessage, QSortFilterProxyModel
*/

/*!
  \enum QMailMessageListModel::Roles

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
    The timestamp of a message. "Recieved" or "Sent" is prepended to the timestamp string depending on the message
    direction.
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
    Constructs a QMailMessageListModel with a parent \a parent.

    By default, the model will match all messages in the database, and display them in
    the order they were submitted, and mail store updates are not ignored.

    \sa setKey(), setSortKey(), setIgnoreMailStoreUpdates()
*/

QMailMessageListModel::QMailMessageListModel(QObject* parent)
:
    QAbstractListModel(parent),
    d(new QMailMessageListModelPrivate(*this,QMailMessageKey::nonMatchingKey(),QMailMessageSortKey::id(),false))
{
    connect(QMailStore::instance(),
            SIGNAL(messagesAdded(QMailMessageIdList)),
            this,
            SLOT(messagesAdded(QMailMessageIdList)));
    connect(QMailStore::instance(),
            SIGNAL(messagesRemoved(QMailMessageIdList)),
            this,
            SLOT(messagesRemoved(QMailMessageIdList)));
    connect(QMailStore::instance(),
            SIGNAL(messagesUpdated(QMailMessageIdList)),
            this,
            SLOT(messagesUpdated(QMailMessageIdList)));
}

/*!
    Deletes the QMailMessageListModel object.
*/

QMailMessageListModel::~QMailMessageListModel()
{
    delete d; d = 0;
}

/*!
    \reimp
*/

int QMailMessageListModel::rowCount(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return d->items().count();
}

/*!
    Returns true if the model contains no messages.
*/
bool QMailMessageListModel::isEmpty() const
{
    return d->items().isEmpty();
}

/*!
    \reimp
*/

QVariant QMailMessageListModel::data(const QModelIndex& index, int role) const
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

    if(!index.isValid())
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
        return (d->itemList[index.row()].isChecked() ? Qt::Checked : Qt::Unchecked);
        break;

    default:
        break;
    }

    // Otherwise, load the message data
    QMailMessageMetaData message(id);

    bool sent(message.status() & QMailMessage::Sent);
    bool incoming(message.status() & QMailMessage::Incoming);

    switch(role)
    {
        case Qt::DisplayRole:
        case MessageAddressTextRole:
            return d->messageAddressText(message,incoming);
            break;

        case MessageTimeStampTextRole:
        {
            QString action;
            if(incoming)
                action = tr("Received");
            else
            {
                if(!sent)
                    action = tr("Last edited");
                else
                    action = tr("Sent");
            }

            QDateTime messageTime(message.date().toLocalTime());

            QString date(messageTime.date().toString("dd MMM"));
            QString time(messageTime.time().toString("h:mm"));
            QString sublabel(QString("%1 %2 %3").arg(action).arg(date).arg(time));
            return sublabel;

        }break;

        case MessageSubjectTextRole:
            return message.subject();
        break;

        case MessageFilterTextRole:
            return d->messageAddressText(message,incoming) + " " + message.subject();
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

        }break;

        case MessageDirectionIconRole:
        {
            QIcon mainIcon = incoming ? incomingIcon : outgoingIcon;
            return mainIcon;
        }break;

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
        }break;

        case MessagePresenceIconRole:
        {
            QIcon icon = noPresenceIcon;
            return icon;
        }break;

        case MessageBodyTextRole:
        {
            if ((message.messageType() == QMailMessage::Instant) && !message.subject().isEmpty()) {
                // For IMs that contain only text, the body is replicated in the subject
                return message.subject();
            } else {
                QMailMessage fullMessage(id);

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

/*!
    \reimp
*/

bool QMailMessageListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid()) {
        // The only role we allow to be changed is the check state
        if (role == Qt::CheckStateRole || role == Qt::EditRole) {
            Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());

            // No support for partial checking in this model
            if (state != Qt::PartiallyChecked) {
                int row = index.row();
                if (row < rowCount()) {
                    d->itemList[row].setChecked(state == Qt::Checked);
                    emit dataChanged(index, index);
                    return true;
                }
            }
        }
    }

    return false;
}

/*!
    Returns the QMailMessageKey used to populate the contents of this model.
*/

QMailMessageKey QMailMessageListModel::key() const
{
    return d->key; 
}

/*!
    Sets the QMailMessageKey used to populate the contents of the model to \a key.
    If the key is empty, the model is populated with all the messages from the 
    database.
*/

void QMailMessageListModel::setKey(const QMailMessageKey& key) 
{
    d->key = key;
    fullRefresh(true);
}

/*!
    Returns the QMailMessageSortKey used to sort the contents of the model.
*/

QMailMessageSortKey QMailMessageListModel::sortKey() const
{
   return d->sortKey;
}

/*!
    Sets the QMailMessageSortKey used to sort the contents of the model to \a sortKey.
    If the sort key is invalid, no sorting is applied to the model contents and messages
    are displayed in the order in which they were added into the database.
*/

void QMailMessageListModel::setSortKey(const QMailMessageSortKey& sortKey) 
{
    // We need a sort key defined, to preserve the ordering in DB records for addition/removal events
    if (sortKey.isEmpty()) {
        d->sortKey = QMailMessageSortKey::id();
    } else {
        d->sortKey = sortKey;
    }

    fullRefresh(true);
}

/*! \internal */

void QMailMessageListModel::messagesAdded(const QMailMessageIdList& ids)
{
    if (!d->init) {
        return;
    }
    
    if (d->ignoreUpdates) {
        d->needSynchronize = true;
        return;
    }

    QList<QMailMessageListModelPrivate::LocationSequence> locations;
    QMailMessageIdList insertIds;

    // Find where these messages should be added
    if (!d->additionLocations(ids, &locations, &insertIds)) {
        // We need to refresh the entire content
        fullRefresh(false);
    } else if (!locations.isEmpty()) {
        QMailMessageIdList::const_iterator it = insertIds.begin();

        foreach (const QMailMessageListModelPrivate::LocationSequence &seq, locations) {
            const QPair<int, int> rows(seq.second);

            // Insert the item(s) at this location
            beginInsertRows(seq.first, rows.first, rows.second);
            for (int i = rows.first; i <= rows.second; ++i) {
                d->insertItemAt(i, seq.first, *it);
                ++it;
            }
            endInsertRows();
        }
    }
}

/*! \internal */

void QMailMessageListModel::messagesUpdated(const QMailMessageIdList& ids)
{
    if (!d->init) {
        return;
    }
    
    if (d->ignoreUpdates) {
        d->needSynchronize = true;
        return;
    }

    QList<QMailMessageListModelPrivate::LocationSequence> insertions;
    QList<QMailMessageListModelPrivate::LocationSequence> removals;
    QList<QMailMessageListModelPrivate::LocationSequence> updates;
    QMailMessageIdList insertIds;

    // Find where these messages should be added/removed/updated
    if (!d->updateLocations(ids, &insertions, &removals, &updates, &insertIds)) {
        // We need to refresh the entire content
        fullRefresh(false);
    } else {
        // Remove any items that are no longer present
        if (!removals.isEmpty()) {
            // Remove the items in reverse order
            for (int n = removals.count(); n > 0; --n) {
                const QMailMessageListModelPrivate::LocationSequence &seq(removals.at(n - 1));
                const QPair<int, int> rows(seq.second);

                beginRemoveRows(seq.first, rows.first, rows.second);
                for (int i = rows.second; i >= rows.first; --i) {
                    d->removeItemAt(i, seq.first);
                }
                endRemoveRows();
            }
        }

        // Insert any new items
        QMailMessageIdList::const_iterator it = insertIds.begin();

        foreach (const QMailMessageListModelPrivate::LocationSequence &seq, insertions) {
            const QPair<int, int> rows(seq.second);

            // Insert the item(s) at this location
            beginInsertRows(seq.first, rows.first, rows.second);
            for (int i = rows.first; i <= rows.second; ++i) {
                d->insertItemAt(i, seq.first, *it);
                ++it;
            }
            endInsertRows();
        }

        // Update any items still at the same location
        foreach (const QMailMessageListModelPrivate::LocationSequence &seq, insertions) {
            const QPair<int, int> rows(seq.second);

            // Update the item(s) at this location
            for (int i = rows.first; i <= rows.second; ++i) {
                QModelIndex idx(index(i, 0, seq.first));
                emit dataChanged(idx, idx);
            }
        }
    }
}

/*! \internal */

void QMailMessageListModel::messagesRemoved(const QMailMessageIdList& ids)
{
    if (!d->init) {
        return;
    }
    
    if (d->ignoreUpdates) {
        d->needSynchronize = true;
        return;
    }

    QList<QMailMessageListModelPrivate::LocationSequence> locations;

    // Find where these messages should be removed from
    if (!d->removalLocations(ids, &locations)) {
        // We need to refresh the entire content
        fullRefresh(false);
    } else if (!locations.isEmpty()) {
        // Remove the items in reverse order
        for (int n = locations.count(); n > 0; --n) {
            const QMailMessageListModelPrivate::LocationSequence &seq(locations.at(n - 1));
            const QPair<int, int> rows(seq.second);

            beginRemoveRows(seq.first, rows.first, rows.second);
            for (int i = rows.second; i >= rows.first; --i) {
                d->removeItemAt(i, seq.first);
            }
            endRemoveRows();
        }
    }
}

/*!
    Returns the QMailMessageId of the message represented by the QModelIndex \a index.
    If the index is not valid an invalid QMailMessageId is returned.
*/

QMailMessageId QMailMessageListModel::idFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return QMailMessageId();

    int row = index.row();
    if (row >= rowCount()) {
        qWarning() << "QMailMessageListModel: valid index" << row << "is out of bounds:" << rowCount();
        return QMailMessageId();
    }

    return d->items()[row].id();
}

/*!
    Returns the QModelIndex that represents the message with QMailMessageId \a id.
    If the id is not conatained in this model, an invalid QModelIndex is returned.
*/

QModelIndex QMailMessageListModel::indexFromId(const QMailMessageId& id) const
{
    if (id.isValid()) {
        //if the id does not exist return null
        int index = d->indexOf(id);
        if(index != -1)
            return createIndex(index,0);
    }

    return QModelIndex();
}

/*!
    Returns true if the model has been set to ignore updates emitted by 
    the mail store; otherwise returns false.
*/
bool QMailMessageListModel::ignoreMailStoreUpdates() const
{
    return d->ignoreUpdates;
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
void QMailMessageListModel::setIgnoreMailStoreUpdates(bool ignore)
{
    d->ignoreUpdates = ignore;
    if (!ignore && d->needSynchronize)
        fullRefresh(false);
}

/*!
    \fn QMailMessageListModel::modelChanged()

    Signal emitted when the data set represented by the model is changed. Unlike modelReset(),
    the modelChanged() signal can not be emitted as a result of changes occurring in the 
    current data set.
*/

/*! \internal */

void QMailMessageListModel::fullRefresh(bool changed) 
{
    d->init = false;
    reset();

    if (changed)
        emit modelChanged();
}

