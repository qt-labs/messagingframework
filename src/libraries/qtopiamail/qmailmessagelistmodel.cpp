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
#include <QCache>
#include <QtAlgorithms>


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

    QMailMessageKey key() const;
    void setKey(const QMailMessageKey& key);

    QMailMessageSortKey sortKey() const;
    void setSortKey(const QMailMessageSortKey& sortKey);

    bool isEmpty() const;
    int rowCount(const QModelIndex& idx) const;

    QMailMessageId idFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromId(const QMailMessageId& id) const;

    Qt::CheckState checkState(const QModelIndex &idx) const;
    void setCheckState(const QModelIndex &idx, Qt::CheckState state);

    void reset();

    bool ignoreMailStoreUpdates() const;
    bool setIgnoreMailStoreUpdates(bool ignore);

    bool additionLocations(const QMailMessageIdList &ids,
                           QList<LocationSequence> *locations, 
                           QMailMessageIdList *insertIds);

    bool updateLocations(const QMailMessageIdList &ids, 
                         QList<LocationSequence> *additions, 
                         QList<LocationSequence> *deletions, 
                         QList<LocationSequence> *updates,
                         QMailMessageIdList *insertIds);

    bool removalLocations(const QMailMessageIdList &ids, 
                          QList<LocationSequence> *locations);

    void insertItemAt(int row, const QModelIndex &parentIndex, const QMailMessageId &id);
    void removeItemAt(int row, const QModelIndex &parentIndex);

private:
    void init() const;

    int indexOf(const QMailMessageId& id) const;

    QList<QMailMessageListModelPrivate::LocationSequence> indicesToLocationSequence(const QList<int> &indices) const;

    QMailMessageListModel &_model;
    QMailMessageKey _key;
    QMailMessageSortKey _sortKey;
    bool _ignoreUpdates;
    mutable QList<Item> _itemList;
    mutable QMap<QMailMessageId, int> _itemIndex;
    mutable QMailMessageIdList _currentIds;
    mutable bool _initialised;
    mutable bool _needSynchronize;
};


QMailMessageListModelPrivate::QMailMessageListModelPrivate(QMailMessageListModel& model,
                                                           const QMailMessageKey& key,
                                                           const QMailMessageSortKey& sortKey,
                                                           bool ignoreUpdates)
:
    _model(model),
    _key(key),
    _sortKey(sortKey),
    _ignoreUpdates(ignoreUpdates),
    _initialised(false),
    _needSynchronize(true)
{
}

QMailMessageListModelPrivate::~QMailMessageListModelPrivate()
{
}

QMailMessageKey QMailMessageListModelPrivate::key() const
{
    return _key; 
}

void QMailMessageListModelPrivate::setKey(const QMailMessageKey& key) 
{
    _key = key;
}

QMailMessageSortKey QMailMessageListModelPrivate::sortKey() const
{
   return _sortKey;
}

void QMailMessageListModelPrivate::setSortKey(const QMailMessageSortKey& sortKey) 
{
    _sortKey = sortKey;
}

bool QMailMessageListModelPrivate::isEmpty() const
{
    init();

    return _itemList.isEmpty();
}

int QMailMessageListModelPrivate::rowCount(const QModelIndex &idx) const
{
    init();

    if (idx.isValid()) {
        // We don't have a hierarchy in this model
        return 0;
    }

    return _itemList.count();
}

QMailMessageId QMailMessageListModelPrivate::idFromIndex(const QModelIndex& index) const
{
    init();

    if (index.isValid()) {
        int row = index.row();
        if ((row >= 0) && (row < _itemList.count())) {
            return _itemList.at(row).id();
        }
    }

    return QMailMessageId();
}

QModelIndex QMailMessageListModelPrivate::indexFromId(const QMailMessageId& id) const
{
    init();

    if (id.isValid()) {
        int row = indexOf(id);
        if (row != -1)
            return _model.generateIndex(row, QModelIndex());
    }

    return QModelIndex();
}

Qt::CheckState QMailMessageListModelPrivate::checkState(const QModelIndex &idx) const
{
    if (idx.isValid()) {
        int row = idx.row();
        if ((row >= 0) && (row < _itemList.count())) {
            return (_itemList.at(row).isChecked() ? Qt::Checked : Qt::Unchecked);
        }
    }

    return Qt::Unchecked;
}

void QMailMessageListModelPrivate::setCheckState(const QModelIndex &idx, Qt::CheckState state)
{
    if (idx.isValid()) {
        int row = idx.row();
        if ((row >= 0) && (row < _itemList.count())) {
            // No support for partial checking in this model...
            _itemList[row].setChecked(state == Qt::Checked);
        }
    }
}

void QMailMessageListModelPrivate::reset()
{
    _initialised = false;
}

bool QMailMessageListModelPrivate::ignoreMailStoreUpdates() const
{
    return _ignoreUpdates;
}

bool QMailMessageListModelPrivate::setIgnoreMailStoreUpdates(bool ignore)
{
    _ignoreUpdates = ignore;
    return (!_ignoreUpdates && _needSynchronize);
}

bool QMailMessageListModelPrivate::additionLocations(const QMailMessageIdList &ids,
                                                     QList<LocationSequence> *locations, 
                                                     QMailMessageIdList *insertIds)
{
    if (!_initialised) {
        // Nothing to do yet
        return true;
    }
    
    if (_ignoreUpdates) {
        // Defer until resynchronised
        _needSynchronize = true;
        return true;
    }

    // Are any of these messages members of our display set?
    // Note - we must only consider messages in the set given by (those we currently know +
    // those we have now been informed of) because the database content may have changed between
    // when this event was recorded and when we're processing the signal.
    
    QMailMessageKey idKey(QMailMessageKey::id(_currentIds + ids));
    QMailMessageIdList newIds(QMailStore::instance()->queryMessages(_key & idKey, _sortKey));

    int additionCount = newIds.count() - _itemList.count();
    if (additionCount <= 0) {
        // Nothing has been added
        return true;
    }

    // Find the locations for these messages by comparing to the existing list
    QList<QMailMessageId>::const_iterator nit = newIds.begin(), nend = newIds.end();
    QList<Item>::const_iterator iit = _itemList.begin(), iend = _itemList.end();

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
                                                   QMailMessageIdList *insertIds)
{
    if (!_initialised) {
        // Nothing to do yet
        return true;
    }
    
    if (_ignoreUpdates) {
        // Defer until resynchronised
        _needSynchronize = true;
        return true;
    }

    QList<int> insertIndices;
    QList<int> removeIndices;
    QList<int> updateIndices;

    // Find the updated positions for our messages
    QMailMessageKey idKey(QMailMessageKey::id((_currentIds.toSet() + ids.toSet()).toList()));
    QMailMessageIdList newIds(QMailStore::instance()->queryMessages(_key & idKey, _sortKey));
    QMap<QMailMessageId, int> newPositions;

    int index = 0;
    foreach (const QMailMessageId &id, newIds) {
        newPositions.insert(id, index);
        ++index;
    }

    int delta = (newIds.count() - _itemList.count());

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

bool QMailMessageListModelPrivate::removalLocations(const QMailMessageIdList &ids, QList<LocationSequence> *locations)
{
    if (!_initialised) {
        // Nothing to do yet
        return true;
    }
    
    if (_ignoreUpdates) {
        // Defer until resynchronised
        _needSynchronize = true;
        return true;
    }

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
    _itemList.insert(row, QMailMessageListModelPrivate::Item(id));
    _itemIndex.insert(id, row);
    _currentIds.append(id);

    // Adjust the indices for the items that have been moved
    QList<Item>::iterator it = _itemList.begin() + (row + 1), end = _itemList.end();
    for ( ; it != end; ++it) {
        _itemIndex[(*it).id()] += 1;
    }

    Q_UNUSED(parentIndex)
}

void QMailMessageListModelPrivate::removeItemAt(int row, const QModelIndex &parentIndex)
{
    QMailMessageId id(_itemList.at(row).id());
    _itemIndex.remove(id);
    _itemList.removeAt(row);
    _currentIds.append(id);

    // Adjust the indices for the items that have been moved
    QList<Item>::iterator it = _itemList.begin() + row, end = _itemList.end();
    for ( ; it != end; ++it) {
        _itemIndex[(*it).id()] -= 1;
    }

    Q_UNUSED(parentIndex)
}

void QMailMessageListModelPrivate::init() const
{
    if (!_initialised) {
        _itemList.clear();
        _itemIndex.clear();

        int index = 0;
        _currentIds = QMailStore::instance()->queryMessages(_key, _sortKey);
        foreach (const QMailMessageId &id, _currentIds) {
            _itemList.append(QMailMessageListModelPrivate::Item(id, false));
            _itemIndex.insert(id, index);
            ++index;
        }

        _initialised = true;
        _needSynchronize = false;
    }
}

int QMailMessageListModelPrivate::indexOf(const QMailMessageId& id) const
{
    QMap<QMailMessageId, int>::const_iterator it = _itemIndex.find(id);
    if (it != _itemIndex.end()) {
        return it.value();
    }

    return -1;
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
    connect(QMailStore::instance(), SIGNAL(messagesAdded(QMailMessageIdList)), this, SLOT(messagesAdded(QMailMessageIdList)));
    connect(QMailStore::instance(), SIGNAL(messagesRemoved(QMailMessageIdList)), this, SLOT(messagesRemoved(QMailMessageIdList)));
    connect(QMailStore::instance(), SIGNAL(messagesUpdated(QMailMessageIdList)), this, SLOT(messagesUpdated(QMailMessageIdList)));
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
    return d->rowCount(index);
}

/*!
    Returns true if the model contains no messages.
*/
bool QMailMessageListModel::isEmpty() const
{
    return d->isEmpty();
}

/*!
    \reimp
*/

QVariant QMailMessageListModel::data(const QModelIndex& index, int role) const
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
        return d->checkState(index);
        break;

    default:
        break;
    }

    // Otherwise, load the message data
    return QMailMessageModelBase::data(QMailMessageMetaData(id), role);
}

/*!
    \reimp
*/

bool QMailMessageListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid()) {
        // The only role we allow to be changed is the check state
        if (role == Qt::CheckStateRole || role == Qt::EditRole) {
            d->setCheckState(index, static_cast<Qt::CheckState>(value.toInt()));
        }
    }

    return false;
}

/*!
    Returns the QMailMessageKey used to populate the contents of this model.
*/

QMailMessageKey QMailMessageListModel::key() const
{
    return d->key(); 
}

/*!
    Sets the QMailMessageKey used to populate the contents of the model to \a key.
    If the key is empty, the model is populated with all the messages from the 
    database.
*/

void QMailMessageListModel::setKey(const QMailMessageKey& key) 
{
    d->setKey(key);
    fullRefresh(true);
}

/*!
    Returns the QMailMessageSortKey used to sort the contents of the model.
*/

QMailMessageSortKey QMailMessageListModel::sortKey() const
{
   return d->sortKey();
}

/*!
    Sets the QMailMessageSortKey used to sort the contents of the model to \a sortKey.
    If the sort key is invalid, no sorting is applied to the model contents and messages
    are displayed in the order in which they were added into the database.
*/

void QMailMessageListModel::setSortKey(const QMailMessageSortKey& sortKey) 
{
    // We need a sort key defined, to preserve the ordering in DB records for addition/removal events
    d->setSortKey(sortKey.isEmpty() ? QMailMessageSortKey::id() : sortKey);
    fullRefresh(true);
}

/*! \internal */

void QMailMessageListModel::messagesAdded(const QMailMessageIdList& ids)
{
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
    return d->idFromIndex(index);
}

/*!
    Returns the QModelIndex that represents the message with QMailMessageId \a id.
    If the id is not contained in this model, an invalid QModelIndex is returned.
*/

QModelIndex QMailMessageListModel::indexFromId(const QMailMessageId& id) const
{
    return d->indexFromId(id);
}

/*!
    Returns true if the model has been set to ignore updates emitted by 
    the mail store; otherwise returns false.
*/
bool QMailMessageListModel::ignoreMailStoreUpdates() const
{
    return d->ignoreMailStoreUpdates();
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
    if (d->setIgnoreMailStoreUpdates(ignore))
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
    d->reset();
    reset();

    if (changed)
        emit modelChanged();
}

/*! \internal */

QModelIndex QMailMessageListModel::generateIndex(int row, const QModelIndex &idx)
{
    return createIndex(row, 0);
}

