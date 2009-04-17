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

    QMailMessageListModelPrivate(const QMailMessageKey& key,
                                 const QMailMessageSortKey& sortKey,
                                 bool sychronizeEnabled);
    ~QMailMessageListModelPrivate();

    const QList<Item>& items() const;

    int indexOf(const QMailMessageId& id) const;

    template<typename Comparator>
    QList<Item>::iterator lowerBound(const QMailMessageId& id, Comparator& cmp) const;

    void invalidateCache();

public:
    QMailMessageKey key;
    QMailMessageSortKey sortKey;
    bool ignoreUpdates;
    mutable QList<Item> itemList;
    mutable bool init;
    mutable bool needSynchronize;
    QCache<QString,QString> nameCache;
};

class ListMessageLessThan
{
public:
    typedef QMailMessageListModelPrivate::Item Item;

    ListMessageLessThan(const QMailMessageSortKey& sortKey);
    ~ListMessageLessThan();

    bool operator()(const QMailMessageId& lhs, const QMailMessageId& rhs);

    bool operator()(const Item& lhs, const Item& rhs) { return operator()(lhs.id(), rhs.id()); }
    bool operator()(const Item& lhs, const QMailMessageId& rhs) { return operator()(lhs.id(), rhs); }
    bool operator()(const QMailMessageId& lhs, const Item& rhs) { return operator()(lhs, rhs.id()); }

    bool invalidatedList() const;

private:
    QMailMessageSortKey mSortKey;
    bool mInvalidatedList;
};

ListMessageLessThan::ListMessageLessThan(const QMailMessageSortKey& sortKey)
:
    mSortKey(sortKey),
    mInvalidatedList(false)
{
}

ListMessageLessThan::~ListMessageLessThan(){}

bool ListMessageLessThan::operator()(const QMailMessageId& lhs, const QMailMessageId& rhs)
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

bool ListMessageLessThan::invalidatedList() const
{
    return mInvalidatedList;
}

QMailMessageListModelPrivate::QMailMessageListModelPrivate(const QMailMessageKey& key,
                                                           const QMailMessageSortKey& sortKey,
                                                           bool ignoreUpdates)
:
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
    if(!init)
    {
        itemList.clear();
        QMailMessageIdList ids = QMailStore::instance()->queryMessages(key,sortKey);
        foreach (const QMailMessageId& id, ids)
            itemList.append(QMailMessageListModelPrivate::Item(id, false));

        init = true;
        needSynchronize = false;
    }

    return itemList;
}

int QMailMessageListModelPrivate::indexOf(const QMailMessageId& id) const
{
    Item item(id, false);

    // Will return the matching item regardless of boolean state due to Item::operator==
    return itemList.indexOf(item);
}

template<typename Comparator>
QList<QMailMessageListModelPrivate::Item>::iterator QMailMessageListModelPrivate::lowerBound(const QMailMessageId& id, Comparator& cmp) const
{
    return qLowerBound(itemList.begin(), itemList.end(), id, cmp);
}

void QMailMessageListModelPrivate::invalidateCache()
{
    nameCache.clear();
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
    d(new QMailMessageListModelPrivate(QMailMessageKey::nonMatchingKey(),QMailMessageSortKey(),false))
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
        return (d->itemList[index.row()].isChecked() ? Qt::Checked : Qt::Unchecked);
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
    d->sortKey = sortKey;
    fullRefresh(true);
}

/*! \internal */

void QMailMessageListModel::messagesAdded(const QMailMessageIdList& ids)
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
            ListMessageLessThan lessThan(d->sortKey);

            //if sorting the list fails, then resort to a complete refresh
            if(lessThan.invalidatedList())
                fullRefresh(false);
            else
            {
                QList<QMailMessageListModelPrivate::Item>::iterator itr = d->lowerBound(id, lessThan);
                int newIndex = (itr - d->itemList.begin());

                beginInsertRows(QModelIndex(),newIndex,newIndex);
                d->itemList.insert(newIndex, QMailMessageListModelPrivate::Item(id));
                endInsertRows();
            }
        }
    }
    else
    {
        int index = d->itemList.count();

        beginInsertRows(QModelIndex(),index,(index + validIds.count() - 1));
        foreach(const QMailMessageId &id,validIds)
            d->itemList.append(QMailMessageListModelPrivate::Item(id));
        endInsertRows();
    }
    d->needSynchronize = false;
}

/*! \internal */

void QMailMessageListModel::messagesUpdated(const QMailMessageIdList& ids)
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
            d->itemList.removeAt(index);
            endRemoveRows();
        }
    }

    ListMessageLessThan lessThan(d->sortKey);

    foreach(const QMailMessageId &id, validIds)
    {
        int index = d->indexOf(id);
        if(index == -1) //insert
        {
            if(lessThan.invalidatedList())
                fullRefresh(false);
            else
            {
                QList<QMailMessageListModelPrivate::Item>::iterator itr = d->lowerBound(id, lessThan);
                int newIndex = (itr - d->itemList.begin());

                beginInsertRows(QModelIndex(),newIndex,newIndex);
                d->itemList.insert(itr, QMailMessageListModelPrivate::Item(id));
                endInsertRows();
            }

        }
        else //update
        {
            if(lessThan.invalidatedList())
                fullRefresh(false);
            else
            {
                QList<QMailMessageListModelPrivate::Item>::iterator itr = d->lowerBound(id, lessThan);
                int newIndex = (itr - d->itemList.begin());

                if((newIndex == index) || (newIndex == index + 1))
                {
                    // This item would be inserted either immediately before or after itself
                    QModelIndex modelIndex = createIndex(index,0);
                    emit dataChanged(modelIndex,modelIndex);
                }
                else
                {
                    beginRemoveRows(QModelIndex(),index,index);
                    d->itemList.removeAt(index);
                    endRemoveRows();

                    if (newIndex > index)
                        --newIndex;

                    beginInsertRows(QModelIndex(),newIndex,newIndex);
                    d->itemList.insert(newIndex, QMailMessageListModelPrivate::Item(id));
                    endInsertRows();
                }
            }
        }
    }
    d->needSynchronize = false;
}

/*! \internal */

void QMailMessageListModel::messagesRemoved(const QMailMessageIdList& ids)
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
            d->itemList.removeAt(index);
            endRemoveRows();
        }
    }

    d->needSynchronize = false;
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
        // Ensure that we have initialised
        d->items();

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

