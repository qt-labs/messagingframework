/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailmessagethreadedmodel.h"
#include "qmailstore.h"
#include "qmailnamespace.h"
#include <QCache>
#include <QtAlgorithms>

static const int fullRefreshCutoff = 10;

class QMailMessageThreadedModelItem
{
public:
    explicit QMailMessageThreadedModelItem(const QMailMessageId& id, QMailMessageThreadedModelItem *parent = 0) : _id(id), _parent(parent) {}
    ~QMailMessageThreadedModelItem() {}

    QMailMessageId id() const { return _id; }

    QMailMessageThreadedModelItem &parent() const { return *_parent; }

    QList<QMailMessageThreadedModelItem> &children() { return _children; }
    const QList<QMailMessageThreadedModelItem> &children() const { return _children; }

    bool operator== (const QMailMessageThreadedModelItem& other) { return (_id == other._id); }

    QMailMessageId _id;
    QMailMessageThreadedModelItem *_parent;
    QList<QMailMessageThreadedModelItem> _children;
};

class QMailMessageThreadedModelPrivate : public QMailMessageModelImplementation
{
public:
    QMailMessageThreadedModelPrivate(QMailMessageThreadedModel& model, const QMailMessageKey& key, const QMailMessageSortKey& sortKey, bool sychronizeEnabled);
    ~QMailMessageThreadedModelPrivate();

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

    int rootRow(const QModelIndex& idx) const;

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

    QModelIndex index(int row, int column, const QModelIndex &parentIndex) const;
    QModelIndex parent(const QModelIndex &idx) const;

private:
    void init() const;

    QModelIndex index(QMailMessageThreadedModelItem *item, int column) const;
    QModelIndex parentIndex(QMailMessageThreadedModelItem *item, int column) const;

    QMailMessageThreadedModelItem *itemFromIndex(const QModelIndex &index) const;
    QModelIndex indexFromItem(QMailMessageThreadedModelItem *item) const;

    QMailMessageThreadedModel &_model;
    QMailMessageKey _key;
    QMailMessageSortKey _sortKey;
    bool _ignoreUpdates;
    mutable QMailMessageThreadedModelItem _root;
    mutable QMap<QMailMessageId, QMailMessageThreadedModelItem*> _messageItem;
    mutable QSet<QMailMessageId> _checkedIds;
    mutable bool _initialised;
    mutable bool _needSynchronize;
};


QMailMessageThreadedModelPrivate::QMailMessageThreadedModelPrivate(QMailMessageThreadedModel& model,
                                                                   const QMailMessageKey& key, 
                                                                   const QMailMessageSortKey& sortKey, 
                                                                   bool ignoreUpdates)
:
    _model(model),
    _key(key),
    _sortKey(sortKey),
    _ignoreUpdates(ignoreUpdates),
    _root(QMailMessageId()),
    _initialised(false),
    _needSynchronize(true)
{
}

QMailMessageThreadedModelPrivate::~QMailMessageThreadedModelPrivate()
{
}

QMailMessageKey QMailMessageThreadedModelPrivate::key() const
{
    return _key; 
}

void QMailMessageThreadedModelPrivate::setKey(const QMailMessageKey& key) 
{
    _key = key;
}

QMailMessageSortKey QMailMessageThreadedModelPrivate::sortKey() const
{
   return _sortKey;
}

void QMailMessageThreadedModelPrivate::setSortKey(const QMailMessageSortKey& sortKey) 
{
    _sortKey = sortKey;
}

bool QMailMessageThreadedModelPrivate::isEmpty() const
{
    init();

    return _root.children().isEmpty();
}

int QMailMessageThreadedModelPrivate::rowCount(const QModelIndex &idx) const
{
    init();

    if (idx.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(idx)) {
            return item->children().count();
        }
    } else {
        return _root.children().count();
    }

    return -1;
}

int QMailMessageThreadedModelPrivate::columnCount(const QModelIndex &idx) const
{
    init();

    return 1;
    
    Q_UNUSED(idx)
}

QMailMessageId QMailMessageThreadedModelPrivate::idFromIndex(const QModelIndex& idx) const
{
    init();

    if (idx.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(idx)) {
            return item->_id;
        }
    }

    return QMailMessageId();
}

QModelIndex QMailMessageThreadedModelPrivate::indexFromId(const QMailMessageId& id) const
{
    init();

    if (id.isValid()) {
        QMap<QMailMessageId, QMailMessageThreadedModelItem*>::const_iterator it = _messageItem.find(id);
        if (it != _messageItem.end()) {
            return indexFromItem(it.value());
        }
    }

    return QModelIndex();
}

Qt::CheckState QMailMessageThreadedModelPrivate::checkState(const QModelIndex &idx) const
{
    init();

    if (idx.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(idx)) {
            return (_checkedIds.contains(item->_id) ? Qt::Checked : Qt::Unchecked);
        }
    }

    return Qt::Unchecked;
}

void QMailMessageThreadedModelPrivate::setCheckState(const QModelIndex &idx, Qt::CheckState state)
{
    if (idx.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(idx)) {
            // No support for partial checking in this model...
            if (state == Qt::Checked) {
                _checkedIds.insert(item->_id);
            } else {
                _checkedIds.remove(item->_id);
            }
        }
    }
}

int QMailMessageThreadedModelPrivate::rootRow(const QModelIndex& idx) const
{
    if (idx.isValid()) {
        QMailMessageThreadedModelItem *item = itemFromIndex(idx);
        while (item->_parent != &_root) {
            item = item->_parent;
        }

        return _root.children().indexOf(*item);
    }

    return -1;
}

void QMailMessageThreadedModelPrivate::reset()
{
    _initialised = false;
}

bool QMailMessageThreadedModelPrivate::ignoreMailStoreUpdates() const
{
    return _ignoreUpdates;
}

bool QMailMessageThreadedModelPrivate::setIgnoreMailStoreUpdates(bool ignore)
{
    _ignoreUpdates = ignore;
    return (!_ignoreUpdates && _needSynchronize);
}

bool QMailMessageThreadedModelPrivate::additionLocations(const QMailMessageIdList &ids,
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

#if 0
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
#endif
    return true;
}

bool QMailMessageThreadedModelPrivate::updateLocations(const QMailMessageIdList &ids, 
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

#if 0
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
#endif
    return true;
}

bool QMailMessageThreadedModelPrivate::removalLocations(const QMailMessageIdList &ids, QList<LocationSequence> *locations)
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

#if 0
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
#endif
    return true;
}

void QMailMessageThreadedModelPrivate::insertItemAt(int row, const QModelIndex &parentIndex, const QMailMessageId &id)
{
#if 0
    _idList.insert(row, id);
    _itemIndex.insert(id, row);

    // Adjust the indices for the items that have been moved
    QList<QMailMessageId>::iterator it = _idList.begin() + (row + 1), end = _idList.end();
    for ( ; it != end; ++it) {
        _itemIndex[*it] += 1;
    }

    Q_UNUSED(parentIndex)
#endif
}

void QMailMessageThreadedModelPrivate::removeItemAt(int row, const QModelIndex &parentIndex)
{
#if 0
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
#endif
}

QModelIndex QMailMessageThreadedModelPrivate::index(int row, int column, const QModelIndex &parentIndex) const
{
    init();

    QMailMessageThreadedModelItem *parent;

    if (parentIndex.isValid()) {
        parent = itemFromIndex(parentIndex);
    } else {
        parent = &_root;
    }

    if (parent && parent->children().count() > row)
        return _model.generateIndex(row, column, reinterpret_cast<void*>(const_cast<QMailMessageThreadedModelItem*>(&(parent->children().at(row)))));

    return QModelIndex();
}

QModelIndex QMailMessageThreadedModelPrivate::parent(const QModelIndex &idx) const
{
    init();

    if (QMailMessageThreadedModelItem *item = itemFromIndex(idx))
        return parentIndex(item, idx.column());

    return QModelIndex();
}

void QMailMessageThreadedModelPrivate::init() const
{
    if (!_initialised) {
        _root.children().clear();
        _messageItem.clear();

        // Find all messages involved in conversations with the messages to show, along with their predecessor ID
        QMailMessageKey conversationKey(QMailMessageKey::conversation(_key));
        QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::InResponseTo);

        QMap<QMailMessageId, QMailMessageId> predecessor;
        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(conversationKey, props)) {
            predecessor.insert(metaData.id(), metaData.inResponseTo());
        }

        // Now find all the messages we're going to show, in order
        QMailMessageIdList ids = QMailStore::instance()->queryMessages(_key, _sortKey);

        // Process the messages to build a tree
        foreach (const QMailMessageId& id, ids) {
            // See if we have already added this message
            QMap<QMailMessageId, QMailMessageThreadedModelItem*>::iterator it = _messageItem.find(id);
            if (it != _messageItem.end())
                continue;

            QMailMessageId messageId(id);
            QList<QMailMessageId> descendants;

            // Find the first message ancestor that is in our display set
            QMailMessageId predecessorId(predecessor[messageId]);
            while (predecessorId.isValid() && !ids.contains(predecessorId)) {
                predecessorId = predecessor[predecessorId];
            }

            do {
                bool added(false);

                if (!predecessorId.isValid()) {
                    // This message is a root node
                    QList<QMailMessageThreadedModelItem> &container(_root.children());
                    container.append(QMailMessageThreadedModelItem(messageId, &_root));
                    _messageItem[messageId] = &(container.last());
                    added = true;
                } else {
                    // Find the predecessor and add to the tree
                    it = _messageItem.find(predecessorId);
                    if (it != _messageItem.end()) {
                        // Add this message to its parent
                        QMailMessageThreadedModelItem *parent(it.value());
                        QList<QMailMessageThreadedModelItem> &container(parent->children());

                        container.append(QMailMessageThreadedModelItem(messageId, parent));
                        _messageItem[messageId] = &(container.last());
                        added = true;
                    }
                }

                if (added) {
                    if (descendants.isEmpty()) {
                        messageId = QMailMessageId();
                    } else {
                        // Process the descendants of the added node
                        predecessorId = messageId;
                        messageId = descendants.takeLast();
                    }
                } else {
                    // We need to add the parent before we can be added to it
                    descendants.append(messageId);

                    messageId = predecessorId;
                    predecessorId = predecessor[messageId];
                    while (predecessorId.isValid() && !ids.contains(predecessorId)) {
                        predecessorId = predecessor[predecessorId];
                    }
                }
            } while (messageId.isValid());
        }

        _initialised = true;
        _needSynchronize = false;
    }
}

QModelIndex QMailMessageThreadedModelPrivate::parentIndex(QMailMessageThreadedModelItem *item, int column) const
{
    if (QMailMessageThreadedModelItem *parent = item->_parent)
        if (parent->_parent != 0)
            return index(static_cast<QMailMessageThreadedModelItem*>(parent), column);

    return QModelIndex();
}

QModelIndex QMailMessageThreadedModelPrivate::index(QMailMessageThreadedModelItem *item, int column) const
{
    if (QMailMessageThreadedModelItem *parent = item->_parent)
        return _model.generateIndex(parent->children().indexOf(*item), column, reinterpret_cast<void*>(item));

    return QModelIndex();
}

QMailMessageThreadedModelItem *QMailMessageThreadedModelPrivate::itemFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return static_cast<QMailMessageThreadedModelItem*>(index.internalPointer());

    return 0;
}

QModelIndex QMailMessageThreadedModelPrivate::indexFromItem(QMailMessageThreadedModelItem *item) const
{
    //return const_cast<QMailMessageThreadedModel*>(this)->index(item, 0);
    return index(item, 0);
}


/*!
    Constructs a QMailMessageThreadedModel with a parent \a parent.

    By default, the model will match all messages in the database, and display them in
    the order they were submitted, and mail store updates are not ignored.

    \sa setKey(), setSortKey(), setIgnoreMailStoreUpdates()
*/
QMailMessageThreadedModel::QMailMessageThreadedModel(QObject* parent)
    : QMailMessageModelBase(parent),
      d(new QMailMessageThreadedModelPrivate(*this, QMailMessageKey::nonMatchingKey(), QMailMessageSortKey::id(), false))
{
}

/*!
    Deletes the QMailMessageThreadedModel object.
*/
QMailMessageThreadedModel::~QMailMessageThreadedModel()
{
    delete d; d = 0;
}

// TODO - is this useful?  Maybe "QMailMessageId rootAncestor()" instead?
int QMailMessageThreadedModel::rootRow(const QModelIndex& idx) const
{
    return d->rootRow(idx);
}

/*! \reimp */
QModelIndex QMailMessageThreadedModel::index(int row, int column, const QModelIndex &parentIndex) const
{
    return d->index(row, column, parentIndex);
}

/*! \reimp */
QModelIndex QMailMessageThreadedModel::parent(const QModelIndex &idx) const
{
    return d->parent(idx);
}

/*! \reimp */
QModelIndex QMailMessageThreadedModel::generateIndex(int row, int column, void *ptr)
{
    return createIndex(row, column, ptr);
}

/*! \reimp */
QMailMessageModelImplementation *QMailMessageThreadedModel::impl()
{
    return d;
}

/*! \reimp */
const QMailMessageModelImplementation *QMailMessageThreadedModel::impl() const
{
    return d;
}

