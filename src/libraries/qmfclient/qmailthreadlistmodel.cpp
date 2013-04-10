/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmailthreadlistmodel.h"
#include "qmailstore.h"
#include "qmailthread.h"
#include <QCache>

class LessThanFunctorT
{
public:
    LessThanFunctorT(const QMailThreadSortKey& sortKey);
    ~LessThanFunctorT();

    bool operator()(const QMailThreadId& first, const QMailThreadId& second);
    bool invalidatedList() const;

private:
    QMailThreadSortKey mSortKey;
    bool mInvalidatedList;
};

LessThanFunctorT::LessThanFunctorT(const QMailThreadSortKey& sortKey)
:
    mSortKey(sortKey),
    mInvalidatedList(false)
{
}

LessThanFunctorT::~LessThanFunctorT(){}

bool LessThanFunctorT::operator()(const QMailThreadId& first, const QMailThreadId& second)
{
    QMailThreadKey firstKey(QMailThreadKey::id(first));
    QMailThreadKey secondKey(QMailThreadKey::id(second));

    QMailThreadIdList results = QMailStore::instance()->queryThreads(firstKey | secondKey, mSortKey);
    if(results.count() != 2)
    {
        mInvalidatedList = true;
        return false;
    }
    return results.first() == first;
}

bool LessThanFunctorT::invalidatedList() const
{
    return mInvalidatedList;
}


class QMailThreadListModelPrivate
{
public:
    QMailThreadListModelPrivate(const QMailThreadKey& key,
                                const QMailThreadSortKey& sortKey,
                                bool synchronizeEnabled);
    ~QMailThreadListModelPrivate();

    void initialize() const;
    const QMailThreadIdList& ids() const;

    int indexOf(const QMailThreadId& id) const;

    template<typename Comparator>
    QMailThreadIdList::iterator lowerBound(const QMailThreadId& id, Comparator& cmp) const;

public:
    QMailThreadKey key;
    QMailThreadSortKey sortKey;
    bool synchronizeEnabled;
    mutable QMailThreadIdList idList;
    mutable QMailThreadId deletionId;
    mutable bool init;
    mutable bool needSynchronize;
};

QMailThreadListModelPrivate::QMailThreadListModelPrivate(const QMailThreadKey& key,
                                                         const QMailThreadSortKey& sortKey,
                                                         bool synchronizeEnabled)
:
    key(key),
    sortKey(sortKey),
    synchronizeEnabled(synchronizeEnabled),
    init(false),
    needSynchronize(true)
{
}

QMailThreadListModelPrivate::~QMailThreadListModelPrivate()
{
}

void QMailThreadListModelPrivate::initialize() const
{
    idList = QMailStore::instance()->queryThreads(key,sortKey);
    init = true;
    needSynchronize = false;
}

const QMailThreadIdList& QMailThreadListModelPrivate::ids() const
{
    if (!init) {
        initialize();
    }

    return idList;
}

int QMailThreadListModelPrivate::indexOf(const QMailThreadId& id) const
{
    return ids().indexOf(id);
}

template<typename Comparator>
QMailThreadIdList::iterator QMailThreadListModelPrivate::lowerBound(const QMailThreadId& id, Comparator& cmp) const
{
    return qLowerBound(idList.begin(), idList.end(), id, cmp);
}


/*!
  \class QMailThreadListModel

  \preliminary
  \ingroup messaginglibrary
  \brief The QMailThreadListModel class provides access to a list of stored threads.

  The QMailThreadListModel presents a list of all the threads currently stored in
  the message store. By using the setKey() and sortKey() functions it is possible to have the model
  represent specific user filtered subsets of threads sorted in a particular order.

  The QMailThreadListModel is a descendant of QAbstractListModel, so it is suitable for use with
  the Qt View classes such as QListView to visually represent lists of threads.

  The model listens for changes to the underlying storage system and sychronizes its contents based on
  the setSynchronizeEnabled() setting.

  Threads can be extracted from the view with the idFromIndex() function and the resultant id can be
  used to load a thread from the store.

  For filters or sorting not provided by the QMailThreadListModel it is recommended that
  QSortFilterProxyModel is used to wrap the model to provide custom sorting and filtering.

  \sa QMailThread, QSortFilterProxyModel
*/

/*!
  \enum QMailThreadListModel::Roles

  Represents common display roles of a thread. These roles are used to display common thread elements
  in a view and its attached delegates.

  \value ThreadSubjectRole The subject of the thread
  \value ThreadPreviewRole The preview of the thread
  \value ThreadUnreadCountRole The unread count of the thread
  \value ThreadMessageCountRole The message count of the thread
  \value ThreadSendersRole The senders of the thread
  \value ThreadLastDateRole The last date of the thread
  \value ThreadStartedDateRole The start date of the thread
  \value ThreadIdRole The id of the thread
*/

/*!
    Constructs a QMailThreadListModel with a parent \a parent.
    By default, the model will match all threads in the database, and display them in
    the order they were submitted. Synchronization defaults to true.
*/

QMailThreadListModel::QMailThreadListModel(QObject* parent)
:
    QAbstractListModel(parent),
    d(new QMailThreadListModelPrivate(QMailThreadKey(),QMailThreadSortKey(),true))
{
    connect(QMailStore::instance(),
            SIGNAL(threadsAdded(QMailThreadIdList)),
            this,
            SLOT(threadsAdded(QMailThreadIdList)));
    connect(QMailStore::instance(),
            SIGNAL(threadsRemoved(QMailThreadIdList)),
            this,
            SLOT(threadsRemoved(QMailThreadIdList)));
    connect(QMailStore::instance(),
            SIGNAL(threadsUpdated(QMailThreadIdList)),
            this,
            SLOT(threadsUpdated(QMailThreadIdList)));
}

/*!
    Deletes the QMailThreadListModel object.
*/

QMailThreadListModel::~QMailThreadListModel()
{
    delete d; d = 0;
}

/*!
    \reimp
*/

int QMailThreadListModel::rowCount(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return d->ids().count();
}

/*!
    \reimp
*/

QVariant QMailThreadListModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return QVariant();

    int offset = index.row();
    QMailThreadId id = d->ids().at(offset);
    if (id == d->deletionId)
        return QVariant();

    QMailThread thread(id);

    switch(role)
    {
    case Qt::DisplayRole:
        return thread.subject();
    case ThreadIdRole:
        return thread.id();
    case ThreadSubjectTextRole:
        return thread.subject();
    case ThreadPreviewTextRole:
        return thread.preview();
    case ThreadUnreadCountRole:
        return thread.unreadCount();
    case ThreadMessageCountRole:
        return thread.messageCount();
    case ThreadSendersRole:
    {
        QStringList senders;
        foreach (const QMailAddress& address, thread.senders()) {
            senders << address.name();
        }
        return senders.join(", ");
    }
    case ThreadLastDateRole:
        return thread.lastDate().toLocalTime();
    case ThreadStartedDateRole:
        return thread.startedDate().toLocalTime();
    }

    return QVariant();
}

/*!
    Returns the QMailThreadKey used to populate the contents of this model.
*/

QMailThreadKey QMailThreadListModel::key() const
{
    return d->key;
}

/*!
    Sets the QMailThreadKey used to populate the contents of the model to \a key.
    If the key is empty, the model is populated with all the threads from the
    database.
*/

void QMailThreadListModel::setKey(const QMailThreadKey& key)
{
    beginResetModel();
    d->key = key;
    d->init = false;
    endResetModel();
}

/*!
    Returns the QMailThreadSortKey used to sort the contents of the model.
*/

QMailThreadSortKey QMailThreadListModel::sortKey() const
{
   return d->sortKey;
}

/*!
    Sets the QMailThreadSortKey used to sort the contents of the model to \a sortKey.
    If the sort key is invalid, no sorting is applied to the model contents and threads
    are displayed in the order in which they were added into the database.
*/

void QMailThreadListModel::setSortKey(const QMailThreadSortKey& sortKey)
{
    beginResetModel();
    d->sortKey = sortKey;
    d->init = false;
    endResetModel();
}

/*! \internal */

void QMailThreadListModel::threadsAdded(const QMailThreadIdList& ids)
{
    d->needSynchronize = true;
    if(!d->synchronizeEnabled)
        return;

    //TODO change this code to use our own searching and insertion routines
    //for more control
    //use id sorted indexes

    if(!d->init)
        d->initialize();

    QMailThreadKey passKey = d->key & QMailThreadKey::id(ids);
    QMailThreadIdList results = QMailStore::instance()->queryThreads(passKey);
    if(results.isEmpty())
        return;

    if(!d->sortKey.isEmpty())
    {
        foreach(const QMailThreadId &id,results)
        {
            LessThanFunctorT lessThan(d->sortKey);

            //if sorting the list fails, then resort to a complete refresh
            if(lessThan.invalidatedList())
                fullRefresh();
            else
            {
                QMailThreadIdList::iterator itr = d->lowerBound(id, lessThan);
                int newIndex = (itr - d->idList.begin());

                beginInsertRows(QModelIndex(),newIndex,newIndex);
                d->idList.insert(itr, id);
                endInsertRows();
            }
        }
    }
    else
    {
        int index = d->idList.count();

        beginInsertRows(QModelIndex(),index,(index + results.count() - 1));
        foreach(const QMailThreadId &id,results)
            d->idList.append(id);
        endInsertRows();
    }
    d->needSynchronize = false;
}

/*! \internal */

void QMailThreadListModel::threadsUpdated(const QMailThreadIdList& ids)
{
    d->needSynchronize = true;
    if(!d->synchronizeEnabled)
        return;

    //TODO change this code to use our own searching and insertion routines
    //for more control
    //use id sorted indexes

    if(!d->init)
        d->initialize();

    QMailThreadKey idKey(QMailThreadKey::id(ids));

    QMailThreadIdList validIds = QMailStore::instance()->queryThreads(idKey & d->key);

    //if the key is empty the id's will be returned valid and invalid
    if(!d->key.isEmpty())
    {
        QMailThreadIdList invalidIds = QMailStore::instance()->queryThreads(idKey & ~d->key);
        foreach(const QMailThreadId &id,invalidIds)
        {
            //get the index
            int index = d->idList.indexOf(id);
            if (index == -1)
                continue;

            d->deletionId = id;
            beginRemoveRows(QModelIndex(),index,index);
            d->idList.removeAt(index);
            endRemoveRows();
            d->deletionId = QMailThreadId();
        }
    }

    LessThanFunctorT lessThan(d->sortKey);

    foreach(const QMailThreadId &id, validIds)
    {
        int index = d->idList.indexOf(id);
        if(index == -1) //insert
        {
            if(lessThan.invalidatedList())
                fullRefresh();
            else
            {
                QMailThreadIdList::iterator itr = d->lowerBound(id, lessThan);
                int newIndex = (itr - d->idList.begin());

                beginInsertRows(QModelIndex(),newIndex,newIndex);
                d->idList.insert(itr, id);
                endInsertRows();
            }
        }
        else //update
        {
            if(lessThan.invalidatedList())
                fullRefresh();
            else
            {
                QMailThreadIdList::iterator itr = d->lowerBound(id, lessThan);
                int newIndex = (itr - d->idList.begin());

                if((newIndex == index) || (newIndex == index + 1))
                {
                    // This item would be inserted either immediately before or after itself
                    QModelIndex modelIndex = createIndex(index,0);
                    emit dataChanged(modelIndex,modelIndex);
                }
                else
                {
                    d->deletionId = id;
                    beginRemoveRows(QModelIndex(),index,index);
                    d->idList.removeAt(index);
                    endRemoveRows();
                    d->deletionId = QMailThreadId();

                    if (newIndex > index)
                        --newIndex;

                    beginInsertRows(QModelIndex(),newIndex,newIndex);
                    d->idList.insert(newIndex, id);
                    endInsertRows();
                }
            }
        }
    }
    d->needSynchronize = false;
}

/*! \internal */

void QMailThreadListModel::threadsRemoved(const QMailThreadIdList& ids)
{
    d->needSynchronize = true;
    if(!d->synchronizeEnabled)
        return;

    if(!d->init)
        d->initialize();

    foreach(const QMailThreadId &id, ids)
    {
        int index = d->indexOf(id);
        if(index == -1)
            continue;

        d->deletionId = id;
        beginRemoveRows(QModelIndex(),index,index);
        d->idList.removeAt(index);
        endRemoveRows();
        d->deletionId = QMailThreadId();
    }
    d->needSynchronize = false;
}

/*!
    Returns the QMailThreadId of the thread represented by the QModelIndex \a index.
    If the index is not valid an invalid QMailThreadId is returned.
*/

QMailThreadId QMailThreadListModel::idFromIndex(const QModelIndex& index) const
{
    if(!index.isValid())
        return QMailThreadId();

    return d->ids().at(index.row());
}

/*!
    Returns the QModelIndex that represents the thread with QMailThreadId \a id.
    If the id is not conatained in this model, an invalid QModelIndex is returned.
*/

QModelIndex QMailThreadListModel::indexFromId(const QMailThreadId& id) const
{
    //if the id does not exist return null
    int index = d->indexOf(id);
    if(index != -1) {
        return createIndex(index,0);
    }

    return QModelIndex();
}

const QMailThreadIdList & QMailThreadListModel::ids() const
{
    return d->ids();
}

/*!
    Returns \c true if the model sychronizes its contents based on thread changes
    in the database, otherwise returns \c false.
*/

bool QMailThreadListModel::synchronizeEnabled() const
{
    return d->synchronizeEnabled;
}

/*!
    Sets wheather the model synchronizes its contents based on thread changes
    in the database to \a val.
*/

void QMailThreadListModel::setSynchronizeEnabled(bool val)
{
    d->synchronizeEnabled = val;
    if(val && d->needSynchronize)
        fullRefresh();
}

/*! \internal */

void QMailThreadListModel::fullRefresh()
{
    beginResetModel();
    d->init = false;
    endResetModel();
    emit modelChanged();
}

