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
#include "qmailnamespace.h"
#include "qmailstore.h"
#include <QtAlgorithms>


class QMailMessageListModelPrivate : public QMailMessageModelImplementation
{
public:
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
    int columnCount(const QModelIndex& idx) const;

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

    QMailMessageListModel &_model;
    QMailMessageKey _key;
    QMailMessageSortKey _sortKey;
    bool _ignoreUpdates;
    mutable QList<QMailMessageId> _idList;
    mutable QMap<QMailMessageId, int> _itemIndex;
    mutable QSet<QMailMessageId> _checkedIds;
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

    return _idList.isEmpty();
}

int QMailMessageListModelPrivate::rowCount(const QModelIndex &idx) const
{
    init();

    if (idx.isValid()) {
        // We don't have a hierarchy in this model
        return 0;
    }

    return _idList.count();
}

int QMailMessageListModelPrivate::columnCount(const QModelIndex &idx) const
{
    init();

    return 1;
    
    Q_UNUSED(idx)
}

QMailMessageId QMailMessageListModelPrivate::idFromIndex(const QModelIndex& index) const
{
    init();

    if (index.isValid()) {
        int row = index.row();
        if ((row >= 0) && (row < _idList.count())) {
            return _idList.at(row);
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
            return _model.generateIndex(row, 0, 0);
    }

    return QModelIndex();
}

Qt::CheckState QMailMessageListModelPrivate::checkState(const QModelIndex &idx) const
{
    if (idx.isValid()) {
        int row = idx.row();
        if ((row >= 0) && (row < _idList.count())) {
            return (_checkedIds.contains(_idList.at(row)) ? Qt::Checked : Qt::Unchecked);
        }
    }

    return Qt::Unchecked;
}

void QMailMessageListModelPrivate::setCheckState(const QModelIndex &idx, Qt::CheckState state)
{
    if (idx.isValid()) {
        int row = idx.row();
        if ((row >= 0) && (row < _idList.count())) {
            // No support for partial checking in this model...
            if (state == Qt::Checked) {
                _checkedIds.insert(_idList.at(row));
            } else {
                _checkedIds.remove(_idList.at(row));
            }
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
    
    QMailMessageKey idKey(QMailMessageKey::id(_idList + ids));
    QMailMessageIdList newIds(QMailStore::instance()->queryMessages(_key & idKey, _sortKey));

    int additionCount = newIds.count() - _idList.count();
    if (additionCount <= 0) {
        // Nothing has been added
        return true;
    }

    // Find the locations for these messages by comparing to the existing list
    QList<QMailMessageId>::const_iterator nit = newIds.begin(), nend = newIds.end();
    QList<QMailMessageId>::const_iterator iit = _idList.begin(), iend = _idList.end();

    QList<int> insertIndices;
    QMap<int, QMailMessageId> indexId;
    for (int index = 0; nit != nend; ++nit, ++index) {
        const QMailMessageId &id(*nit);

        if ((iit == iend) || (*iit != id)) {
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
    QMailMessageKey idKey(QMailMessageKey::id((_idList.toSet() + ids.toSet()).toList()));
    QMailMessageIdList newIds(QMailStore::instance()->queryMessages(_key & idKey, _sortKey));
    QMap<QMailMessageId, int> newPositions;

    int index = 0;
    foreach (const QMailMessageId &id, newIds) {
        newPositions.insert(id, index);
        ++index;
    }

    int delta = (newIds.count() - _idList.count());

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
    _idList.insert(row, id);
    _itemIndex.insert(id, row);

    // Adjust the indices for the items that have been moved
    QList<QMailMessageId>::iterator it = _idList.begin() + (row + 1), end = _idList.end();
    for ( ; it != end; ++it) {
        _itemIndex[*it] += 1;
    }

    Q_UNUSED(parentIndex)
}

void QMailMessageListModelPrivate::removeItemAt(int row, const QModelIndex &parentIndex)
{
    QMailMessageId id(_idList.at(row));
    _checkedIds.remove(id);
    _itemIndex.remove(id);
    _idList.removeAt(row);

    // Adjust the indices for the items that have been moved
    QList<QMailMessageId>::iterator it = _idList.begin() + row, end = _idList.end();
    for ( ; it != end; ++it) {
        _itemIndex[*it] -= 1;
    }

    Q_UNUSED(parentIndex)
}

void QMailMessageListModelPrivate::init() const
{
    if (!_initialised) {
        _idList.clear();
        _itemIndex.clear();
        _checkedIds.clear();

        int index = 0;
        _idList = QMailStore::instance()->queryMessages(_key, _sortKey);
        foreach (const QMailMessageId &id, _idList) {
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
    : QMailMessageModelBase(parent),
      d(new QMailMessageListModelPrivate(*this, QMailMessageKey::nonMatchingKey(), QMailMessageSortKey::id(), false))
{
}

/*!
    Deletes the QMailMessageListModel object.
*/
QMailMessageListModel::~QMailMessageListModel()
{
    delete d; d = 0;
}

/*! \reimp */
QModelIndex QMailMessageListModel::index(int row, int column, const QModelIndex &idx) const
{
    if (idx.isValid()) {
        return QModelIndex();
    }

    return createIndex(row, column);
}

/*! \reimp */
QModelIndex QMailMessageListModel::parent(const QModelIndex &idx) const
{
    return QModelIndex();

    Q_UNUSED(idx)
}

/*! \reimp */
QModelIndex QMailMessageListModel::generateIndex(int row, int column, void *ptr)
{
    return index(row, column, QModelIndex());

    Q_UNUSED(ptr)
}

/*! \reimp */
QMailMessageModelImplementation *QMailMessageListModel::impl()
{
    return d;
}

/*! \reimp */
const QMailMessageModelImplementation *QMailMessageListModel::impl() const
{
    return d;
}

