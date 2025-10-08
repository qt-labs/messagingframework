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

#include "qmailmessagethreadedmodel.h"
#include "qmailstore.h"
#include "qmailnamespace.h"
#include "qmaillog.h"
#include "qmailid.h"

#include <QList>
#include <QtAlgorithms>

class QMailMessageThreadedModelItem
{
public:
    explicit QMailMessageThreadedModelItem(const QMailMessageId& id, QMailMessageThreadedModelItem *parent = Q_NULLPTR)
        : _id(id), _parent(parent) {}
    ~QMailMessageThreadedModelItem() {}

    int rowInParent() const
    {
        int i = 0;
        for (auto &v : _parent->_children) {
            if (*this == v)
                return i;
            else
                i++;
        }
        return -1;
    }

    QMailMessageIdList childrenIds() const
    {
        QMailMessageIdList ids;
        foreach (const QMailMessageThreadedModelItem &item, _children) {
            ids.append(item._id);
        }

        return ids;
    }

    bool operator== (const QMailMessageThreadedModelItem& other) const { return (_id == other._id); }

    QMailMessageId _id;
    QMailMessageThreadedModelItem *_parent;
    // TODO: replace the related functionality with something more readable, robust and better performing
    std::list<QMailMessageThreadedModelItem> _children;
};


class QMailMessageThreadedModelPrivate : public QMailMessageModelImplementation
{
public:
    QMailMessageThreadedModelPrivate(QMailMessageThreadedModel& model, const QMailMessageKey& key, const QMailMessageSortKey& sortKey, bool sychronizeEnabled);
    ~QMailMessageThreadedModelPrivate();

    QMailMessageKey key() const override;
    void setKey(const QMailMessageKey& key) override;

    QMailMessageSortKey sortKey() const override;
    void setSortKey(const QMailMessageSortKey& sortKey) override;

    uint limit() const override;
    void setLimit(uint limit) override;
    int totalCount() const override;

    bool isEmpty() const override;

    int rowCount(const QModelIndex& idx) const override;
    int columnCount(const QModelIndex& idx) const override;

    QMailMessageId idFromIndex(const QModelIndex& index) const override;
    QModelIndex indexFromId(const QMailMessageId& id) const override;

    Qt::CheckState checkState(const QModelIndex &idx) const override;
    void setCheckState(const QModelIndex &idx, Qt::CheckState state) override;

    int rootRow(const QModelIndex& idx) const;

    void reset() override;

    bool ignoreMailStoreUpdates() const override;
    bool setIgnoreMailStoreUpdates(bool ignore) override;

    bool processMessagesAdded(const QMailMessageIdList &ids) override;
    bool processMessagesUpdated(const QMailMessageIdList &ids) override;
    bool processMessagesRemoved(const QMailMessageIdList &ids) override;

    bool addMessages(const QMailMessageIdList &ids);
    bool appendMessages(const QMailMessageIdList &idsToAppend, const QMailMessageIdList &newIdsList);
    bool updateMessages(const QMailMessageIdList &ids);
    bool removeMessages(const QMailMessageIdList &ids, QMailMessageIdList *readditions);

    void insertItemAt(int row, const QModelIndex &parentIndex, const QMailMessageId &id);
    void removeItemAt(int row, const QModelIndex &parentIndex);

    QModelIndex index(int row, int column, const QModelIndex &parentIndex) const;
    QModelIndex parent(const QModelIndex &idx) const;

private:
    void init() const;

    QModelIndex index(const QMailMessageThreadedModelItem *item, int column) const;
    QModelIndex parentIndex(const QMailMessageThreadedModelItem *item) const;

    QMailMessageThreadedModelItem *itemFromIndex(const QModelIndex &index) const;
    QModelIndex indexFromItem(const QMailMessageThreadedModelItem *item) const;

    QMailMessageThreadedModel &_model;
    QMailMessageKey _key;
    QMailMessageSortKey _sortKey;
    bool _ignoreUpdates;
    mutable QMailMessageThreadedModelItem _root;
    mutable QMap<QMailMessageId, QMailMessageThreadedModelItem*> _messageItem;
    mutable QSet<QMailMessageId> _checkedIds;
    mutable QList<QMailMessageId> _currentIds;
    mutable bool _initialised;
    mutable bool _needSynchronize;
    uint _limit;
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
    _needSynchronize(true),
    _limit(0)
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

uint QMailMessageThreadedModelPrivate::limit() const
{
   return _limit;
}

void QMailMessageThreadedModelPrivate::setLimit(uint limit)
{
    if ( _limit != limit) {
        if (limit == 0) {
            // Do full refresh
            _limit = limit;
            _model.fullRefresh(false);
        } else if (_limit > limit) {
            // Limit decreased, remove messages in excess
            _limit = limit;
            QMailMessageIdList idsToRemove = _currentIds.mid(limit);
            removeMessages(idsToRemove, 0);
        } else {
            _limit = limit;
            QMailMessageIdList idsToAppend;
            QMailMessageIdList newIdsList(QMailStore::instance()->queryMessages(_key, _sortKey, _limit));

            foreach (const QMailMessageId &id, newIdsList) {
                if (!_currentIds.contains(id)) {
                    idsToAppend.append(id);
                }
            }
            appendMessages(idsToAppend, newIdsList);
        }
    }
}

int QMailMessageThreadedModelPrivate::totalCount() const
{
    if (_limit) {
        return QMailStore::instance()->countMessages(_key);
    } else {
        init();
        return _root._children.size();
    }
}

bool QMailMessageThreadedModelPrivate::isEmpty() const
{
    init();

    return _root._children.size() == 0;
}

int QMailMessageThreadedModelPrivate::rowCount(const QModelIndex &idx) const
{
    init();

    if (idx.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(idx)) {
            return item->_children.size();
        }
    } else {
        return _root._children.size();
    }

    return -1;
}

int QMailMessageThreadedModelPrivate::columnCount(const QModelIndex &idx) const
{
    Q_UNUSED(idx)
    init();

    return 1;
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

        return item->rowInParent();
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

bool QMailMessageThreadedModelPrivate::processMessagesAdded(const QMailMessageIdList &ids)
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

    // Find if and where these messages should be added
    if (!addMessages(ids)) {
        return false;
    }

    if (!_initialised) {
        init();
    }

    return true;
}

bool QMailMessageThreadedModelPrivate::addMessages(const QMailMessageIdList &ids)
{
    // Are any of these messages members of our display set?
    // Note - we must only consider messages in the set given by (those we currently know +
    // those we have now been informed of) because the database content may have changed between
    // when this event was recorded and when we're processing the signal.

    QMailMessageKey idKey(QMailMessageKey::id(_currentIds + ids));
    const QMailMessageIdList newIdsList(QMailStore::instance()->queryMessages(_key & idKey, _sortKey, _limit));

    return appendMessages(ids, newIdsList);
}

bool QMailMessageThreadedModelPrivate::appendMessages(const QMailMessageIdList &idsToAppend, const QMailMessageIdList &newIdsList)
{
    // Find which of the messages we must add (in ascending insertion order)
    QList<int> validIndices;
    QHash<QMailMessageId, int> idIndexMap;
    foreach (const QMailMessageId &id, idsToAppend) {
        int index = newIdsList.indexOf(id);
        if (index != -1) {
            validIndices.append(index);
            idIndexMap[id] = index;
        }
    }

    if (validIndices.isEmpty())
        return true;

    std::sort(validIndices.begin(), validIndices.end());

    QMailMessageIdList additionIds;
    foreach (int index, validIndices) {
        additionIds.append(newIdsList.at(index));
    }

    // Find all messages involved in conversations with the new messages, along with their predecessor ID
    QMailMessageKey conversationKey(QMailMessageKey::conversation(QMailMessageKey::id(additionIds)));
    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::InResponseTo);

    QMap<QMailMessageId, QMailMessageId> predecessor;
    for (const QMailMessageMetaData &metaData : QMailStore::instance()->messagesMetaData(conversationKey, props)) {
        predecessor.insert(metaData.id(), metaData.inResponseTo());
    }

    // Process the messages to insert into the tree
    foreach (const QMailMessageId& id, additionIds) {
        // See if we have already added this message
        QMap<QMailMessageId, QMailMessageThreadedModelItem*>::iterator it = _messageItem.find(id);
        if (it != _messageItem.end()) {
            continue;
        }

        QMailMessageId messageId(id);
        QList<QMailMessageId> descendants;

        // Find the first message ancestor that is in our display set
        QMailMessageId predecessorId(predecessor[messageId]);
        while (predecessorId.isValid() && !newIdsList.contains(predecessorId)) {
            predecessorId = predecessor[predecessorId];
        }

        do {
            int messagePos = newIdsList.indexOf(messageId);

            QMailMessageThreadedModelItem *insertParent = 0;

            if (!predecessorId.isValid()) {
                // This message is a root node - we need to find where it fits in
                insertParent = &_root;
            } else {
                // Find the predecessor and add to the tree
                it = _messageItem.find(predecessorId);
                if (it != _messageItem.end()) {
                    insertParent = it.value();
                }
            }

            if (descendants.indexOf(messageId) != -1) {
                qCWarning(lcMessaging) << "Conversation loop detected" << Q_FUNC_INFO << "messageId" << messageId << "descendants" << descendants;
                insertParent = &_root;
            }

            if (insertParent != 0) {
                int insertIndex = 0;

                // Find the insert location within the parent
                foreach (const QMailMessageId &id, insertParent->childrenIds()) {
                    int childPos = -1;
                    if (idIndexMap.contains(id)) {
                        childPos = idIndexMap[id];
                    } else {
                        childPos = newIdsList.indexOf(id);
                        idIndexMap[id] = childPos;
                    }
                    if ((childPos != -1) && (childPos < messagePos)) {
                        // Ths child precedes us in the sort order
                        ++insertIndex;
                    }
                }

                QModelIndex parentIndex = indexFromItem(insertParent);

                // Since we insert the parents first, if index is bigger than
                // the limit we stop the loop
                if (_limit && insertIndex > (int)_limit) {
                     break;
                }
                _model.emitBeginInsertRows(parentIndex, insertIndex, insertIndex);
                insertItemAt(insertIndex, parentIndex, messageId);
                _model.emitEndInsertRows();

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
                while (predecessorId.isValid() && !newIdsList.contains(predecessorId)) {
                    predecessorId = predecessor[predecessorId];
                }
            }
        } while (messageId.isValid());
    }

    // Check if we passed the model limit, if so remove exceeding messages
    if (_limit && _currentIds.count() > (int)_limit) {
        QMailMessageIdList idsToRemove = _currentIds.mid(_limit);
        removeMessages(idsToRemove, 0);
    }

    return true;
}

bool QMailMessageThreadedModelPrivate::processMessagesUpdated(const QMailMessageIdList &ids)
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

    if (!_initialised) {
        init();
    }

    // Find if and where these messages should be added/removed/updated
    if (!updateMessages(ids)) {
        return false;
    }

    return true;
}

bool QMailMessageThreadedModelPrivate::updateMessages(const QMailMessageIdList &ids)
{
    QSet<QMailMessageId> existingIds(_currentIds.constBegin(), _currentIds.constEnd());
    existingIds.unite(QSet<QMailMessageId>(ids.constBegin(), ids.constEnd()));

    QMailMessageKey idKey(QMailMessageKey::id(existingIds.values()));
    QMailMessageIdList newIds(QMailStore::instance()->queryMessages(_key & idKey, _sortKey, _limit));

    QSet<QMailMessageId> currentIds(newIds.constBegin(), newIds.constEnd());

    // Find which of the messages we must add and remove
    QMailMessageIdList additionIds;
    QMailMessageIdList removalIds;
    QMailMessageIdList temporaryRemovalIds;
    QMailMessageIdList updateIds;

    foreach (const QMailMessageId &id, ids) {
        bool existingMember(existingIds.contains(id));
        bool currentMember(currentIds.contains(id));

        if (!existingMember && currentMember) {
            additionIds.append(id);
        } else if (existingMember && !currentMember) {
            removalIds.append(id);
        } else if (currentMember) {
            updateIds.append(id);
        }
    }

    if (additionIds.isEmpty() && removalIds.isEmpty() && updateIds.isEmpty()) {
        return true;
    }

    // For the updated messages, find if they have a changed predecessor

    // Find all messages involved in conversations with the updated messages, along with their predecessor ID
    QMailMessageKey conversationKey(QMailMessageKey::conversation(QMailMessageKey::id(updateIds)));
    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::InResponseTo);

    QMap<QMailMessageId, QMailMessageId> predecessor;
    for (const QMailMessageMetaData &metaData : QMailStore::instance()->messagesMetaData(conversationKey, props)) {
        predecessor.insert(metaData.id(), metaData.inResponseTo());
    }

    QList<QPair<QModelIndex, int> > updateIndices;

    foreach (const QMailMessageId &messageId, updateIds) {
        // Find the first message ancestor that is in our display set
        QMailMessageId predecessorId(predecessor[messageId]);
        while (predecessorId.isValid() && !currentIds.contains(predecessorId)) {
            predecessorId = predecessor[predecessorId];
        }

        bool reinsert(false);

        QMailMessageThreadedModelItem *item = _messageItem[messageId];
        if (item->_parent == &_root) {
            // This is a root item
            if (predecessorId.isValid()) {
                // This item now has a predecessor
                reinsert = true;
            }
        } else {
            if (item->_parent->_id != predecessorId) {
                // This item has a changed predecessor
                reinsert = true;
            }
        }

        if (!reinsert) {
            // We need to see if this item has changed in the sort order
            int row = item->rowInParent();
            int messagePos = newIds.indexOf(messageId);
            std::list<QMailMessageThreadedModelItem> &container(item->_parent->_children);

            if (row > 0) {
                // Ensure that we still sort after our immediate predecessor
                if (newIds.indexOf(std::next(container.begin(), row - 1)->_id) > messagePos) {
                    reinsert = true;
                }
            }
            if (row < (container.size() - 1)) {
                // Ensure that we still sort before our immediate successor
                if (newIds.indexOf(std::next(container.begin(), row + 1)->_id) < messagePos) {
                    reinsert = true;
                }
            }
        }

        if (reinsert) {
            // Remove and re-add this item
            temporaryRemovalIds.append(messageId);
        } else {
            // Find the location for this updated item (not its parent)
            _model.emitDataChanged(index(item, 0), index(item, _model.columnCount() - 1));
        }
    }

    // Find the locations for removals
    removeMessages(removalIds, 0);

    // TODO To prevent children being orphaned all messages in all conversations involving messages in
    // TODO temporaryRemovalIds should be removed and then added.
    // Find the locations for those to be removed and any children IDs that need to be reinserted after removal
    QMailMessageIdList readditionIds;
    removeMessages(temporaryRemovalIds, &readditionIds);

    // Find the locations for the added and reinserted messages
    QSet<QMailMessageId> uniqueIds(additionIds.constBegin(), additionIds.constEnd());
    uniqueIds.unite(QSet<QMailMessageId>(temporaryRemovalIds.constBegin(), temporaryRemovalIds.constEnd()));
    uniqueIds.unite(QSet<QMailMessageId>(readditionIds.constBegin(), readditionIds.constEnd()));
    addMessages(uniqueIds.values());

    return true;
}

bool QMailMessageThreadedModelPrivate::processMessagesRemoved(const QMailMessageIdList &ids)
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

    if (!_initialised) {
        init();
    }

    // Find if and where these messages should be removed from
    if (!removeMessages(ids, 0)) {
        return false;
    }

    return true;
}

bool QMailMessageThreadedModelPrivate::removeMessages(const QMailMessageIdList &ids, QMailMessageIdList *readditions)
{
    QSet<QMailMessageId> removedIds;
    QSet<QMailMessageId> childIds;

    foreach (const QMailMessageId &id, ids) {
        if (!removedIds.contains(id)) {
            QMap<QMailMessageId, QMailMessageThreadedModelItem*>::iterator it = _messageItem.find(id);
            if (it != _messageItem.end()) {
                QMailMessageThreadedModelItem *item = it.value();
                QModelIndex idx(indexFromItem(item));

                // Find all the children of this item that need to be removed/re-added
                QList<const QMailMessageThreadedModelItem*> items;
                items.append(item);

                while (!items.isEmpty()) {
                    const QMailMessageThreadedModelItem *parent = items.takeFirst();
                    foreach (const QMailMessageThreadedModelItem &child, parent->_children) {
                        if (!removedIds.contains(child._id)) {
                            removedIds.insert(child._id);
                            if (readditions) {
                                // Don't re-add this child if it is being deleted itself
                                if (!ids.contains(child._id)) {
                                    childIds.insert(child._id);
                                }
                            }

                            items.append(&child);
                        }
                    }
                }

                _model.emitBeginRemoveRows(idx.parent(), item->rowInParent(), item->rowInParent());
                removeItemAt(item->rowInParent(), idx.parent());
                removedIds.insert(id);
                _model.emitEndRemoveRows();
            }
        }
    }

    if (readditions) {
        *readditions = childIds.values();
    }

    return true;
}

void QMailMessageThreadedModelPrivate::insertItemAt(int row, const QModelIndex &parentIndex, const QMailMessageId &id)
{
    QMailMessageThreadedModelItem *parent;
    if (parentIndex.isValid()) {
        parent = itemFromIndex(parentIndex);
    } else {
        parent = &_root;
    }

    std::list<QMailMessageThreadedModelItem> &container(parent->_children);

    container.insert(std::next(container.begin(), row), QMailMessageThreadedModelItem(id, parent));
    _messageItem[id] = &(*std::next(container.begin(), row));
    _currentIds.append(id);
}

void QMailMessageThreadedModelPrivate::removeItemAt(int row, const QModelIndex &parentIndex)
{
    QMailMessageThreadedModelItem *parent;
    if (parentIndex.isValid()) {
        parent = itemFromIndex(parentIndex);
    } else {
        parent = &_root;
    }

    std::list<QMailMessageThreadedModelItem> &container(parent->_children);

    if (container.size() > row) {
        QMailMessageThreadedModelItem *item = &(*(std::next(container.begin(), row)));

        // Find all the descendants of this item that no longer exist
        QList<const QMailMessageThreadedModelItem*> items;
        items.append(item);

        while (!items.isEmpty()) {
            const QMailMessageThreadedModelItem *parent = items.takeFirst();
            foreach (const QMailMessageThreadedModelItem &child, parent->_children) {
                QMailMessageId childId(child._id);
                if (_messageItem.contains(childId)) {
                    _checkedIds.remove(childId);
                    _currentIds.removeAll(childId);
                    _messageItem.remove(childId);
                }

                items.append(&child);
            }
        }

        QMailMessageId id(item->_id);

        _checkedIds.remove(id);
        _currentIds.removeAll(id);
        _messageItem.remove(id);
        container.erase(std::next(container.begin(), row));
    }
}

QModelIndex QMailMessageThreadedModelPrivate::index(int row, int column, const QModelIndex &parentIndex) const
{
    init();

    if (row >= 0) {
        QMailMessageThreadedModelItem *parent;
        if (parentIndex.isValid()) {
            parent = itemFromIndex(parentIndex);
        } else {
            parent = &_root;
        }

        // Allow excessive row values (although these indices won't be dereferencable)
        void *item = 0;
        if (parent && (parent->_children.size() > row)) {
            item = static_cast<void*>(const_cast<QMailMessageThreadedModelItem*>(&(*(std::next(parent->_children.begin(), row)))));
        }

        return _model.generateIndex(row, column, item);
    }

    return QModelIndex();
}

QModelIndex QMailMessageThreadedModelPrivate::parent(const QModelIndex &idx) const
{
    init();

    if (QMailMessageThreadedModelItem *item = itemFromIndex(idx))
        return parentIndex(item);

    return QModelIndex();
}

void QMailMessageThreadedModelPrivate::init() const
{
    if (!_initialised) {
        _root._children.clear();
        _messageItem.clear();
        _currentIds.clear();

        // Find all messages involved in conversations with the messages to show, along with their predecessor ID
        QMailMessageKey conversationKey(QMailMessageKey::conversation(_key));
        QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::InResponseTo);

        QMap<QMailMessageId, QMailMessageId> predecessor;
        for (const QMailMessageMetaData &metaData : QMailStore::instance()->messagesMetaData(conversationKey, props)) {
            predecessor.insert(metaData.id(), metaData.inResponseTo());
        }

        // Now find all the messages we're going to show, in order
        const QMailMessageIdList ids = QMailStore::instance()->queryMessages(_key, _sortKey, _limit);
        QHash<QMailMessageId, int> idIndexMap;
        idIndexMap.reserve(ids.count());

        for (int i = 0; i < ids.count(); ++i)
            idIndexMap[ids[i]] = i;

        // Process the messages to build a tree
        for (int i = 0; i < ids.count(); ++i) {
            // See if we have already added this message
            QMap<QMailMessageId, QMailMessageThreadedModelItem*>::iterator it = _messageItem.find(ids[i]);
            if (it != _messageItem.end())
                continue;

            QMailMessageId messageId(ids[i]);

            QList<QMailMessageId> descendants;

            // Find the first message ancestor that is in our display set
            QMailMessageId predecessorId(predecessor[messageId]);
            while (predecessorId.isValid() && !ids.contains(predecessorId)) {
                predecessorId = predecessor[predecessorId];
            }

            do {
                int itemSortValue = idIndexMap[messageId];
                QMailMessageThreadedModelItem *insertParent = nullptr;

                if (!predecessorId.isValid()) {
                    // This message is a root node
                    insertParent = &_root;
                } else {
                    // Find the predecessor and add to the tree
                    it = _messageItem.find(predecessorId);
                    if (it != _messageItem.end()) {
                        // Add this message to its parent
                        insertParent = it.value();
                    }
                }

                if (descendants.indexOf(messageId) != -1) {
                    qCWarning(lcMessaging) << "Conversation loop detected" << Q_FUNC_INFO << "messageId" << messageId << "descendants" << descendants;
                    insertParent = &_root;
                }
                if (insertParent != nullptr) {
                    // Append the message to the existing children of the parent
                    std::list<QMailMessageThreadedModelItem> &container(insertParent->_children);

                    // Determine where this message should sort amongst its siblings
                    int index = container.size();
                    for ( ; index > 0; --index) {
                        if (!idIndexMap.contains(std::next(container.begin(), index - 1)->_id)) {
                            qCWarning(lcMessaging) << "Warning: Threading hash failure" << __FUNCTION__;
                            idIndexMap[std::next(container.begin(), index - 1)->_id] = ids.indexOf(std::next(container.begin(), index - 1)->_id);
                        }
                        if (idIndexMap[std::next(container.begin(), index - 1)->_id] < itemSortValue) {
                            break;
                        }
                    }

                    container.insert(std::next(container.begin(), index), QMailMessageThreadedModelItem(messageId, insertParent));
                    _messageItem[messageId] = &(*(std::next(container.begin(), index)));
                    _currentIds.append(messageId);

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

QModelIndex QMailMessageThreadedModelPrivate::parentIndex(const QMailMessageThreadedModelItem *item) const
{
    if (const QMailMessageThreadedModelItem *parent = item->_parent) {
        if (parent->_parent)
            return index(parent, 0);
    }
    return QModelIndex();
}

QModelIndex QMailMessageThreadedModelPrivate::index(const QMailMessageThreadedModelItem *item, int column) const
{
    if (item->_parent)
        return _model.generateIndex(item->rowInParent(), column, const_cast<void*>(static_cast<const void*>(item)));

    return QModelIndex();
}

QMailMessageThreadedModelItem *QMailMessageThreadedModelPrivate::itemFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return static_cast<QMailMessageThreadedModelItem*>(index.internalPointer());

    return 0;
}

QModelIndex QMailMessageThreadedModelPrivate::indexFromItem(const QMailMessageThreadedModelItem *item) const
{
    return index(item, 0);
}


/*!
    \class QMailMessageThreadedModel

    \preliminary
    \ingroup messaginglibrary
    \brief The QMailMessageThreadedModel class provides access to a tree of stored messages.

    The QMailMessageThreadedModel presents a tree of all the messages currently stored in
    the message store. By using the setKey() and sortKey() functions it is possible to
    have the model represent specific user filtered subsets of messages sorted in a particular order.

    The QMailMessageThreadedModel represents the hierarchical links between messages
    implied by conversation threads.  The model presents messages as children of predecessor
    messages, where the parent is the nearest ancestor of the message that is present in
    the filtered set of messages.
*/

/*!
    Constructs a QMailMessageThreadedModel with a parent \a parent.

    By default, the model will not match any messages, display messages in
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
/*! \internal */
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

/*! \internal */
QModelIndex QMailMessageThreadedModel::generateIndex(int row, int column, void *ptr)
{
    return createIndex(row, column, ptr);
}

/*! \internal */
QMailMessageModelImplementation *QMailMessageThreadedModel::impl()
{
    return d;
}

/*! \internal */
const QMailMessageModelImplementation *QMailMessageThreadedModel::impl() const
{
    return d;
}
