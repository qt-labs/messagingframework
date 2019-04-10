/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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

    uint limit() const;
    void setLimit(uint limit);
    int totalCount() const;

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

    bool processMessagesAdded(const QMailMessageIdList &ids);
    bool processMessagesUpdated(const QMailMessageIdList &ids);
    bool processMessagesRemoved(const QMailMessageIdList &ids);

private:
    void init() const;
    
    int indexOf(const QMailMessageId& id) const;

    bool addMessages(const QMailMessageIdList &ids);
    bool appendMessages(const QMailMessageIdList &idsToAppend, const QMailMessageIdList &newIdsList);
    bool updateMessages(const QMailMessageIdList &ids);
    bool removeMessages(const QMailMessageIdList &ids);

    void insertItemAt(int row, const QModelIndex &parentIndex, const QMailMessageId &id);
    void removeItemAt(int row, const QModelIndex &parentIndex);

    QMailMessageListModel &_model;
    QMailMessageKey _key;
    QMailMessageSortKey _sortKey;
    bool _ignoreUpdates;
    mutable QList<QMailMessageId> _idList;
    mutable QMap<QMailMessageId, int> _itemIndex;
    mutable QSet<QMailMessageId> _checkedIds;
    mutable bool _initialised;
    mutable bool _needSynchronize;
    uint _limit;
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
    _needSynchronize(true),
    _limit(0)
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

uint QMailMessageListModelPrivate::limit() const
{
   return _limit;
}

void QMailMessageListModelPrivate::setLimit(uint limit)
{
    if (_limit != limit) {
        uint oldLimit = _limit;
        _limit = limit;

        if (!_initialised) {
            // nothing, content will be updated when something tries to access the model and init() is triggered
        } else if (limit == 0) {
            // Do full refresh
            _model.fullRefresh(false);
        } else if (oldLimit > limit) {
            // Limit decreased, remove messages in excess
            QMailMessageIdList idsToRemove = _idList.mid(limit);
            removeMessages(idsToRemove);
        } else {
            QMailMessageIdList idsToAppend;
            QMailMessageIdList newIdsList(QMailStore::instance()->queryMessages(_key, _sortKey, _limit));

            foreach (const QMailMessageId &id, newIdsList) {
                if (!_idList.contains(id)) {
                    idsToAppend.append(id);
                }
            }
            appendMessages(idsToAppend, newIdsList);
        }
    }
}

int QMailMessageListModelPrivate::totalCount() const
{
    if (_limit) {
       return QMailStore::instance()->countMessages(_key);
    } else {
        init();
        return _idList.count();
    }
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
    Q_UNUSED(idx)

    init();

    return 1;
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

bool QMailMessageListModelPrivate::processMessagesAdded(const QMailMessageIdList &ids)
{
    if (ids.empty()) {
        return true;
    }

    if (_ignoreUpdates) {
        // Defer until resynchronised
        _needSynchronize = true;
        return true;
    }

    if (_key.isNonMatching()) {
        // No messages are relevant
        return true;
    }

    init();

    // Find if and where these messages should be added
    if (!addMessages(ids)) {
        return false;
    }

    return true;
}

bool QMailMessageListModelPrivate::addMessages(const QMailMessageIdList &ids)
{
    // Are any of these messages members of our display set?
    // Note - we must only consider messages in the set given by (those we currently know +
    // those we have now been informed of) because the database content may have changed between
    // when this event was recorded and when we're processing the signal.

    QMailMessageKey idKey(QMailMessageKey::id(_idList + ids));
    const QMailMessageIdList newIdsList(QMailStore::instance()->queryMessages(_key & idKey, _sortKey, _limit));

    return appendMessages(ids, newIdsList);
}

bool QMailMessageListModelPrivate::appendMessages(const QMailMessageIdList &idsToAppend, const QMailMessageIdList &newIdsList)
{
    QList<int> insertIndices;
    QMap<QMailMessageId, int> newPositions;

    int index = 0;
    foreach (const QMailMessageId &id, newIdsList) {
        newPositions.insert(id, index);
        ++index;
    }

    QMap<int, QMailMessageId> indexId;
    foreach (const QMailMessageId &id, idsToAppend) {
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
        }
    }

    std::sort(insertIndices.begin(), insertIndices.end());
    foreach (int index, insertIndices) {
        // Since the list is ordered, if index is bigger than the limit
        // we stop inserting
        if (_limit && index > (int)_limit) {
            break;
        }
        _model.emitBeginInsertRows(QModelIndex(), index, index);
        insertItemAt(index, QModelIndex(), indexId[index]);
        _model.emitEndInsertRows();
    }

    // Check if we passed the model limit, if so remove exceeding messages
    if (_limit && _idList.count() > (int)_limit) {
        QMailMessageIdList idsToRemove = _idList.mid(_limit);
        removeMessages(idsToRemove);
    }
    return true;
}

bool QMailMessageListModelPrivate::processMessagesUpdated(const QMailMessageIdList &ids)
{
    if (_ignoreUpdates) {
        // Defer until resynchronised
        _needSynchronize = true;
        return true;
    }

    if (_key.isNonMatching()) {
        // No messages are relevant
        return true;
    }

    init();

    // Find if and where these messages should be added/removed/updated
    if (!updateMessages(ids)) {
        return false;
    }
    
    return true;
}

bool QMailMessageListModelPrivate::updateMessages(const QMailMessageIdList &ids)
{
    QList<int> insertIndices;
    QList<int> removeIndices;
    QList<int> updateIndices;

    // Find the updated positions for our messages
    QMailMessageKey idKey(QMailMessageKey::id((_idList.toSet() + ids.toSet()).toList()));
    QMailMessageIdList newIds(QMailStore::instance()->queryMessages(_key & idKey, _sortKey, _limit));
    QMap<QMailMessageId, int> newPositions;

    int index = 0;
    foreach (const QMailMessageId &id, newIds) {
        newPositions.insert(id, index);
        ++index;
    }

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
                bool reinsert(false);

                // See if this item is still sorted correctly with respect to its neighbours
                if (newIndex >= _idList.count()) {
                    qWarning() << "QMailMessageListModelPrivate::updateMessage index out of bounds" << newIndex << _idList.count();
                } else if (newIndex > 0) {
                    if (newIds.indexOf(_idList.at(newIndex - 1)) > newIndex) {
                        reinsert = true;
                    }
                }

                if (newIndex < _idList.count() - 1) {
                    if (newIds.indexOf(_idList.at(newIndex + 1)) < newIndex) {
                        reinsert = true;
                    }
                }

                if (reinsert) {
                    removeIndices.append(oldIndex);
                    insertIndices.append(newIndex);
                    indexId.insert(newIndex, id);
                } else {
                    // This message is updated but has not changed position
                    updateIndices.append(newIndex);
                }
            }
        }
    }

    // Sort the lists to yield ascending order
    std::sort(removeIndices.begin(), removeIndices.end());
    for (int i = removeIndices.count(); i > 0; --i) {
        int index = removeIndices[i - 1];
        _model.emitBeginRemoveRows(QModelIndex(), index, index);
        removeItemAt(index, QModelIndex());
        _model.emitEndRemoveRows();
    }

    std::sort(insertIndices.begin(), insertIndices.end());
    foreach (int index, insertIndices) {
        // Since the list is ordered, if index is bigger than the limit
        // we stop inserting
        if (_limit && index > (int)_limit) {
            break;
        }
        _model.emitBeginInsertRows(QModelIndex(), index, index);
        insertItemAt(index, QModelIndex(), indexId[index]);
        _model.emitEndInsertRows();
    }

    // Check if we passed the model limit, if so remove exceeding messages
    if (_limit && _idList.count() > (int)_limit) {
        QMailMessageIdList idsToRemove = _idList.mid(_limit);
        removeMessages(idsToRemove);
    }

    std::sort(updateIndices.begin(), updateIndices.end());
    foreach (int index, updateIndices) {
        _model.emitDataChanged(_model.index(index, 0, QModelIndex()), _model.index(index, _model.columnCount() - 1, QModelIndex()));
    }

    return true;
}

bool QMailMessageListModelPrivate::processMessagesRemoved(const QMailMessageIdList &ids)
{
    if (_ignoreUpdates) {
        // Defer until resynchronised
        _needSynchronize = true;
        return true;
    }

    if (_key.isNonMatching()) {
        // No messages are relevant
        return true;
    }

    init();
    
    // Find if and where these messages should be removed from
    if (!removeMessages(ids)) {
        return false;
    }

    return true;
}

bool QMailMessageListModelPrivate::removeMessages(const QMailMessageIdList &ids)
{
    QList<int> removeIndices;
    foreach (const QMailMessageId &id, ids) {
        int index(indexOf(id));
        if (index != -1) {
            removeIndices.append(index);
        }
    }
    
    // Sort the indices to yield ascending order (they must be deleted in descending order!)
    std::sort(removeIndices.begin(), removeIndices.end());

    for (int i = removeIndices.count(); i > 0; --i) {
        int index = removeIndices.at(i - 1);
        _model.emitBeginRemoveRows(QModelIndex(), index, index);
        removeItemAt(index, QModelIndex());
        _model.emitEndRemoveRows();
    }

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
        _idList = QMailStore::instance()->queryMessages(_key, _sortKey, _limit);
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

    \preliminary
    \ingroup messaginglibrary 
    \brief The QMailMessageListModel class provides access to a list of stored messages. 

    The QMailMessageListModel presents a list of all the messages currently stored in
    the message store. By using the setKey() and sortKey() functions it is possible to 
    have the model represent specific user filtered subsets of messages sorted in a particular order.

    The QMailMessageListModel does not represent the hierarchical links between messages 
    implied by conversation threads.  The model flattens the structure of messages such
    that they can be presented as a one-dimensional list.
*/

/*!
    Constructs a QMailMessageListModel with a parent \a parent.

    By default, the model will not match any messages, display messages in
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
QModelIndex QMailMessageListModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

/*! \reimp */
QModelIndex QMailMessageListModel::parent(const QModelIndex &idx) const
{
    Q_UNUSED(idx)

    return QModelIndex();
}

/*! \internal */
QModelIndex QMailMessageListModel::generateIndex(int row, int column, void *ptr)
{
    Q_UNUSED(ptr)

    return index(row, column, QModelIndex());
}

/*! \internal */
QMailMessageModelImplementation *QMailMessageListModel::impl()
{
    return d;
}

/*! \internal */
const QMailMessageModelImplementation *QMailMessageListModel::impl() const
{
    return d;
}

