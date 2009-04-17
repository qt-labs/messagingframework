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
    explicit QMailMessageThreadedModelItem(const QMailMessageId& id, QMailMessageThreadedModelItem *parent = 0) : _id(id), _parent(parent), _checked(false) {}
    ~QMailMessageThreadedModelItem() {}

    QMailMessageId id() const { return _id; }

    bool isChecked() const { return _checked; }
    void setChecked(bool f) { _checked = f; }

    QMailMessageThreadedModelItem &parent() const { return *_parent; }

    QList<QMailMessageThreadedModelItem> &children() { return _children; }
    const QList<QMailMessageThreadedModelItem> &children() const { return _children; }

    bool operator== (const QMailMessageThreadedModelItem& other) { return (_id == other._id); }

    QMailMessageId _id;
    QMailMessageThreadedModelItem *_parent;
    QList<QMailMessageThreadedModelItem> _children;
    bool _checked;
};

class QMailMessageThreadedModelPrivate
{
public:
    QMailMessageThreadedModelPrivate(const QMailMessageKey& key, const QMailMessageSortKey& sortKey, bool sychronizeEnabled);
    ~QMailMessageThreadedModelPrivate();

    const QList<QMailMessageThreadedModelItem>& rootItems() const;

#if 0
    int indexOf(const QMailMessageId& id) const;

    template<typename Comparator>
    QList<QMailMessageThreadedModelItem>::iterator lowerBound(const QMailMessageId& id, Comparator& cmp) const;
#endif

public:
    QMailMessageKey key;
    QMailMessageSortKey sortKey;
    bool ignoreUpdates;
    mutable QMailMessageThreadedModelItem root;
    mutable QMap<QMailMessageId, QMailMessageThreadedModelItem*> messageItem;
    mutable bool init;
    mutable bool needSynchronize;
};

class ThreadedMessageLessThan
{
public:
    typedef QMailMessageThreadedModelItem Item;

    ThreadedMessageLessThan(const QMailMessageSortKey& sortKey);
    ~ThreadedMessageLessThan();

    bool operator()(const QMailMessageId& lhs, const QMailMessageId& rhs);

    bool operator()(const Item& lhs, const Item& rhs) { return operator()(lhs.id(), rhs.id()); }
    bool operator()(const Item& lhs, const QMailMessageId& rhs) { return operator()(lhs.id(), rhs); }
    bool operator()(const QMailMessageId& lhs, const Item& rhs) { return operator()(lhs, rhs.id()); }

    bool invalidatedList() const;

private:
    QMailMessageSortKey mSortKey;
    bool mInvalidatedList;
};

ThreadedMessageLessThan::ThreadedMessageLessThan(const QMailMessageSortKey& sortKey)
:
    mSortKey(sortKey),
    mInvalidatedList(false)
{
}

ThreadedMessageLessThan::~ThreadedMessageLessThan(){}

bool ThreadedMessageLessThan::operator()(const QMailMessageId& lhs, const QMailMessageId& rhs)
{
    QMailMessageKey firstKey(QMailMessageKey::id(lhs));
    QMailMessageKey secondKey(QMailMessageKey::id(rhs));

    // TODO: we have to do this in a more efficient manner:
    QMailMessageIdList results = QMailStore::instance()->queryMessages(firstKey | secondKey, mSortKey);
    if(results.count() != 2)
    {
        mInvalidatedList = true;
        return false;
    }
    return results.first() == lhs;
}

bool ThreadedMessageLessThan::invalidatedList() const
{
    return mInvalidatedList;
}

QMailMessageThreadedModelPrivate::QMailMessageThreadedModelPrivate(const QMailMessageKey& key, const QMailMessageSortKey& sortKey, bool ignoreUpdates)
:
    key(key),
    sortKey(sortKey),
    ignoreUpdates(ignoreUpdates),
    root(QMailMessageId()),
    init(false),
    needSynchronize(true)
{
}

QMailMessageThreadedModelPrivate::~QMailMessageThreadedModelPrivate()
{
}

const QList<QMailMessageThreadedModelItem>& QMailMessageThreadedModelPrivate::rootItems() const
{
    if (!init) {
        root.children().clear();
        messageItem.clear();

        // Find all messages involved in conversations with the messages to show, along with their predecessor ID
        QMailMessageKey conversationKey(QMailMessageKey::conversation(key));
        QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::InResponseTo);

        QMap<QMailMessageId, QMailMessageId> predecessor;
        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(conversationKey, props)) {
            predecessor.insert(metaData.id(), metaData.inResponseTo());
        }

        // Now find all the messages we're going to show, in order
        QMailMessageIdList ids = QMailStore::instance()->queryMessages(key, sortKey);

        // Process the messages to build a tree
        foreach (const QMailMessageId& id, ids) {
            // See if we have already added this message
            QMap<QMailMessageId, QMailMessageThreadedModelItem*>::iterator it = messageItem.find(id);
            if (it != messageItem.end())
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
                    QList<QMailMessageThreadedModelItem> &container(root.children());
                    container.append(QMailMessageThreadedModelItem(messageId, &root));
                    messageItem[messageId] = &(container.last());
                    added = true;
                } else {
                    // Find the predecessor and add to the tree
                    it = messageItem.find(predecessorId);
                    if (it != messageItem.end()) {
                        // Add this message to its parent
                        QMailMessageThreadedModelItem *parent(it.value());
                        QList<QMailMessageThreadedModelItem> &container(parent->children());

                        container.append(QMailMessageThreadedModelItem(messageId, parent));
                        messageItem[messageId] = &(container.last());
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

        init = true;
        needSynchronize = false;
    }

    return root.children();
}

#if 0
int QMailMessageThreadedModelPrivate::indexOf(const QMailMessageId& id) const
{
    return root.children().indexOf(QMailMessageThreadedModelItem(id));
}

template<typename Comparator>
QList<QMailMessageThreadedModelItem>::iterator QMailMessageThreadedModelPrivate::lowerBound(const QMailMessageId& id, Comparator& cmp) const
{
    return qLowerBound(root.children(), id, cmp);
}
#endif


/*!
    Constructs a QMailMessageThreadedModel with a parent \a parent.

    By default, the model will match all messages in the database, and display them in
    the order they were submitted, and mail store updates are not ignored.

    \sa setKey(), setSortKey(), setIgnoreMailStoreUpdates()
*/

QMailMessageThreadedModel::QMailMessageThreadedModel(QObject* parent)
:
    QAbstractItemModel(parent),
    d(new QMailMessageThreadedModelPrivate(QMailMessageKey::nonMatchingKey(), QMailMessageSortKey(), false))
{
#if 0
    connect(QMailStore::instance(), SIGNAL(messagesAdded(QMailMessageIdList)), this, SLOT(messagesAdded(QMailMessageIdList)));
    connect(QMailStore::instance(), SIGNAL(messagesRemoved(QMailMessageIdList)), this, SLOT(messagesRemoved(QMailMessageIdList)));
    connect(QMailStore::instance(), SIGNAL(messagesUpdated(QMailMessageIdList)), this, SLOT(messagesUpdated(QMailMessageIdList)));
#endif
}

/*!
    Deletes the QMailMessageThreadedModel object.
*/

QMailMessageThreadedModel::~QMailMessageThreadedModel()
{
    delete d; d = 0;
}

/*!
    \reimp
*/

int QMailMessageThreadedModel::rowCount(const QModelIndex& index) const
{
    if (index.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(index)) {
            return item->children().count();
        }
    } else {
        return d->rootItems().count();
    }

    return -1;
}

/*!
    \reimp
*/

int QMailMessageThreadedModel::columnCount(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return 1;
}

/*!
    Returns true if the model contains no messages.
*/
bool QMailMessageThreadedModel::isEmpty() const
{
    return d->rootItems().isEmpty();
}

/*!
    \reimp
*/

QVariant QMailMessageThreadedModel::data(const QModelIndex& index, int role) const
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
        if (QMailMessageThreadedModelItem *item = itemFromIndex(index)) {
            return (item->isChecked() ? Qt::Checked : Qt::Unchecked);
        }
        return false;
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

bool QMailMessageThreadedModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid()) {
        // The only role we allow to be changed is the check state
        if (role == Qt::CheckStateRole || role == Qt::EditRole) {
            Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());

            // No support for partial checking in this model
            if (state != Qt::PartiallyChecked) {
                if (QMailMessageThreadedModelItem *item = itemFromIndex(index)) {
                    item->setChecked(state == Qt::Checked);
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

QMailMessageKey QMailMessageThreadedModel::key() const
{
    return d->key; 
}

/*!
    Sets the QMailMessageKey used to populate the contents of the model to \a key.
    If the key is empty, the model is populated with all the messages from the 
    database.
*/

void QMailMessageThreadedModel::setKey(const QMailMessageKey& key) 
{
    d->key = key;
    fullRefresh(true);
}

/*!
    Returns the QMailMessageSortKey used to sort the contents of the model.
*/

QMailMessageSortKey QMailMessageThreadedModel::sortKey() const
{
   return d->sortKey;
}

/*!
    Sets the QMailMessageSortKey used to sort the contents of the model to \a sortKey.
    If the sort key is invalid, no sorting is applied to the model contents and messages
    are displayed in the order in which they were added into the database.
*/

void QMailMessageThreadedModel::setSortKey(const QMailMessageSortKey& sortKey) 
{
    d->sortKey = sortKey;
    fullRefresh(true);
}

/*! \internal */

#if 0
void QMailMessageThreadedModel::messagesAdded(const QMailMessageIdList& ids)
{
    d->needSynchronize = true;
    if(d->ignoreUpdates)
        return;

    //TODO change this code to use our own searching and insertion routines
    //for more control
    //use id sorted indexes
    
    if(!d->init)
        return;
    
    QMailMessageKey idKey(QMailMessageKey::id(ids));

    QMailMessageIdList validIds = QMailStore::instance()->queryMessages(idKey & d->key);
    if (validIds.isEmpty()) { 
        return;
    } else if (validIds.count() > fullRefreshCutoff) {
        fullRefresh(false);
        return;
    }

    if(!d->sortKey.isEmpty())
    { 
        foreach(const QMailMessageId &id,validIds)
        {
            ThreadedMessageLessThan lessThan(d->sortKey);

            //if sorting the list fails, then resort to a complete refresh
            if(lessThan.invalidatedList())
                fullRefresh(false);
            else
            {
                QList<QMailMessageThreadedModelItem>::iterator itr = d->lowerBound(id, lessThan);
                int newIndex = (itr - d->rootItems.begin());

                beginInsertRows(QModelIndex(),newIndex,newIndex);
                d->rootItems.insert(newIndex, QMailMessageThreadedModelItem(id));
                endInsertRows();
            }
        }
    }
    else
    {
        int index = d->rootItems.count();

        beginInsertRows(QModelIndex(),index,(index + validIds.count() - 1));
        foreach(const QMailMessageId &id,validIds)
            d->rootItems.append(QMailMessageThreadedModelItem(id));
        endInsertRows();
    }
    d->needSynchronize = false;
}

/*! \internal */

void QMailMessageThreadedModel::messagesUpdated(const QMailMessageIdList& ids)
{
    d->needSynchronize = true;
    if(d->ignoreUpdates)
        return;

    //TODO change this code to use our own searching and insertion routines
    //for more control
    //use id sorted indexes

    if(!d->init)
        return;

    QMailMessageKey idKey(QMailMessageKey::id(ids));

    QMailMessageIdList validIds = QMailStore::instance()->queryMessages(idKey & d->key);
    if (validIds.isEmpty()) { 
        return;
    } else if (validIds.count() > fullRefreshCutoff) {
        fullRefresh(false);
        return;
    }

    //if the key is empty the id's will be returned valid and invalid
    if(!d->key.isEmpty())
    {
        QMailMessageIdList invalidIds = QMailStore::instance()->queryMessages(idKey & ~d->key);

        foreach(const QMailMessageId &id,invalidIds)
        {
            //get the index
            int index = d->indexOf(id);
            if(index == -1) 
                continue;

            beginRemoveRows(QModelIndex(),index,index);
            d->rootItems.removeAt(index);
            endRemoveRows();
        }
    }

    ThreadedMessageLessThan lessThan(d->sortKey);

    foreach(const QMailMessageId &id, validIds)
    {
        int index = d->indexOf(id);
        if(index == -1) //insert
        {
            if(lessThan.invalidatedList())
                fullRefresh(false);
            else
            {
                QList<QMailMessageThreadedModelItem>::iterator itr = d->lowerBound(id, lessThan);
                int newIndex = (itr - d->rootItems.begin());

                beginInsertRows(QModelIndex(),newIndex,newIndex);
                d->rootItems.insert(itr, QMailMessageThreadedModelItem(id));
                endInsertRows();
            }

        }
        else //update
        {
            if(lessThan.invalidatedList())
                fullRefresh(false);
            else
            {
                QList<QMailMessageThreadedModelItem>::iterator itr = d->lowerBound(id, lessThan);
                int newIndex = (itr - d->rootItems.begin());

                if((newIndex == index) || (newIndex == index + 1))
                {
                    // This item would be inserted either immediately before or after itself
                    QModelIndex modelIndex = createIndex(index,0);
                    emit dataChanged(modelIndex,modelIndex);
                }
                else
                {
                    beginRemoveRows(QModelIndex(),index,index);
                    d->rootItems.removeAt(index);
                    endRemoveRows();

                    if (newIndex > index)
                        --newIndex;

                    beginInsertRows(QModelIndex(),newIndex,newIndex);
                    d->rootItems.insert(newIndex, QMailMessageThreadedModelItem(id));
                    endInsertRows();
                }
            }
        }
    }
    d->needSynchronize = false;
}

/*! \internal */

void QMailMessageThreadedModel::messagesRemoved(const QMailMessageIdList& ids)
{
    d->needSynchronize = true;
    if(d->ignoreUpdates)
        return;

    if(!d->init)
        return;

    QList<int> indexes;

    foreach (const QMailMessageId &id, ids) {
        int index = d->indexOf(id);
        if (index != -1 && !indexes.contains(index))
            indexes.append(index);
    }

    if (!indexes.isEmpty()) {
        qSort(indexes.begin(), indexes.end(), qGreater<int>());

        foreach (int index, indexes) {
            beginRemoveRows(QModelIndex(), index, index);
            d->rootItems.removeAt(index);
            endRemoveRows();
        }
    }

    d->needSynchronize = false;
}
#endif

QMailMessageId QMailMessageThreadedModel::idFromIndex(const QModelIndex& index) const
{
    if (index.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(index)) {
            return item->_id;
        }
    }

    return QMailMessageId();
}

QModelIndex QMailMessageThreadedModel::indexFromId(const QMailMessageId& id) const
{
    if (id.isValid()) {
        // Ensure that we're initialised
        d->rootItems();

        QMap<QMailMessageId, QMailMessageThreadedModelItem*>::const_iterator it = d->messageItem.find(id);
        if (it != d->messageItem.end()) {
            return indexFromItem(it.value());
        }
    }

    return QModelIndex();
}

int QMailMessageThreadedModel::rootRow(const QModelIndex& index) const
{
    if (index.isValid()) {
        QMailMessageThreadedModelItem *item = itemFromIndex(index);
        while (item->_parent != &d->root) {
            item = item->_parent;
        }

        return d->root.children().indexOf(*item);
    }

    return -1;
}

QMailMessageThreadedModelItem *QMailMessageThreadedModel::itemFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return static_cast<QMailMessageThreadedModelItem*>(index.internalPointer());

    return 0;
}

QModelIndex QMailMessageThreadedModel::indexFromItem(QMailMessageThreadedModelItem *item) const
{
    return const_cast<QMailMessageThreadedModel*>(this)->index(item, 0);
}

QModelIndex QMailMessageThreadedModel::parentIndex(QMailMessageThreadedModelItem *item, int column) const
{
    if (QMailMessageThreadedModelItem *parent = item->_parent)
        if (parent->_parent != 0)
            return index(static_cast<QMailMessageThreadedModelItem*>(parent), column);

    return QModelIndex();
}

QModelIndex QMailMessageThreadedModel::index(QMailMessageThreadedModelItem *item, int column) const
{
    if (QMailMessageThreadedModelItem *parent = item->_parent)
        return createIndex(parent->children().indexOf(*item), column, item);

    return QModelIndex();
}

QModelIndex QMailMessageThreadedModel::index(int row, int column, const QModelIndex &parentIndex) const
{
    QMailMessageThreadedModelItem *parent;

    if (parentIndex.isValid()) {
        parent = itemFromIndex(parentIndex);
    } else {
        // Ensure that we're initialised
        d->rootItems();

        parent = &d->root;
    }

    if (parent && parent->children().count() > row)
        return createIndex(row, column, reinterpret_cast<void*>(const_cast<QMailMessageThreadedModelItem*>(&(parent->children().at(row)))));

    return QModelIndex();
}

QModelIndex QMailMessageThreadedModel::parent(const QModelIndex &index) const
{
    if (QMailMessageThreadedModelItem *item = itemFromIndex(index))
        return parentIndex(item, index.column());

    return QModelIndex();
}

/*!
    Returns true if the model has been set to ignore updates emitted by 
    the mail store; otherwise returns false.
*/
bool QMailMessageThreadedModel::ignoreMailStoreUpdates() const
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
void QMailMessageThreadedModel::setIgnoreMailStoreUpdates(bool ignore)
{
    d->ignoreUpdates = ignore;
    if (!ignore && d->needSynchronize)
        fullRefresh(false);
}

/*!
    \fn QMailMessageThreadedModel::modelChanged()

    Signal emitted when the data set represented by the model is changed. Unlike modelReset(),
    the modelChanged() signal can not be emitted as a result of changes occurring in the 
    current data set.
*/

/*! \internal */

void QMailMessageThreadedModel::fullRefresh(bool changed) 
{
    d->init = false;
    reset();

    if (changed)
        emit modelChanged();
}

