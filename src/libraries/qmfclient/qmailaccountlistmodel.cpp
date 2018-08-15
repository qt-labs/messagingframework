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

#include "qmailaccountlistmodel.h"
#include "qmailstore.h"
#include "qmailaccount.h"
#include <QCache>

static const int fullRefreshCutoff = 10;

class LessThanFunctorA
{
public:
    LessThanFunctorA(const QMailAccountSortKey& sortKey);
    ~LessThanFunctorA();

    bool operator()(const QMailAccountId& first, const QMailAccountId& second);
    bool invalidatedList() const;

private:
    QMailAccountSortKey mSortKey;
    bool mInvalidatedList;
};

LessThanFunctorA::LessThanFunctorA(const QMailAccountSortKey& sortKey)
:
    mSortKey(sortKey),
    mInvalidatedList(false)
{
}

LessThanFunctorA::~LessThanFunctorA(){}

bool LessThanFunctorA::operator()(const QMailAccountId& first, const QMailAccountId& second)
{
    QMailAccountKey firstKey(QMailAccountKey::id(first));
    QMailAccountKey secondKey(QMailAccountKey::id(second));

    QMailAccountIdList results = QMailStore::instance()->queryAccounts(firstKey | secondKey, mSortKey);
    if(results.count() != 2)
    {
        mInvalidatedList = true;
        return false;
    }
    return results.first() == first;
}

bool LessThanFunctorA::invalidatedList() const
{
    return mInvalidatedList;
}


class QMailAccountListModelPrivate
{
public:
    QMailAccountListModelPrivate(const QMailAccountKey& key,
                                 const QMailAccountSortKey& sortKey,
                                 bool synchronizeEnabled);
    ~QMailAccountListModelPrivate();

    void initialize() const;
    const QMailAccountIdList& ids() const;

    int indexOf(const QMailAccountId& id) const;

    template<typename Comparator>
    QMailAccountIdList::iterator lowerBound(const QMailAccountId& id, Comparator& cmp) const;

public:
    QMailAccountKey key;
    QMailAccountSortKey sortKey;
    bool synchronizeEnabled;
    mutable QMailAccountIdList idList;
    mutable QMailAccountId deletionId;
    mutable bool init;
    mutable bool needSynchronize;
};

QMailAccountListModelPrivate::QMailAccountListModelPrivate(const QMailAccountKey& key,
                                                           const QMailAccountSortKey& sortKey,
                                                           bool synchronizeEnabled)
:
    key(key),
    sortKey(sortKey),
    synchronizeEnabled(synchronizeEnabled),
    init(false),
    needSynchronize(true)
{
}

QMailAccountListModelPrivate::~QMailAccountListModelPrivate()
{
}

void QMailAccountListModelPrivate::initialize() const
{
    idList = QMailStore::instance()->queryAccounts(key,sortKey);
    init = true;
    needSynchronize = false;
}

const QMailAccountIdList& QMailAccountListModelPrivate::ids() const
{
    if (!init) {
        initialize();
    }

    return idList;
}

int QMailAccountListModelPrivate::indexOf(const QMailAccountId& id) const
{
    return ids().indexOf(id);
}

template<typename Comparator>
QMailAccountIdList::iterator QMailAccountListModelPrivate::lowerBound(const QMailAccountId& id, Comparator& cmp) const
{
    return std::lower_bound(idList.begin(), idList.end(), id, cmp);
}


/*!
  \class QMailAccountListModel 

  \preliminary
  \ingroup messaginglibrary 
  \brief The QMailAccountListModel class provides access to a list of stored accounts. 

  The QMailAccountListModel presents a list of all the accounts currently stored in
  the message store. By using the setKey() and sortKey() functions it is possible to have the model
  represent specific user filtered subsets of accounts sorted in a particular order.

  The QMailAccountListModel is a descendant of QAbstractListModel, so it is suitable for use with
  the Qt View classes such as QListView to visually represent lists of accounts. 

  The model listens for changes to the underlying storage system and sychronizes its contents based on
  the setSynchronizeEnabled() setting.
 
  Accounts can be extracted from the view with the idFromIndex() function and the resultant id can be 
  used to load an account from the store. 

  For filters or sorting not provided by the QMailAccountListModel it is recommended that
  QSortFilterProxyModel is used to wrap the model to provide custom sorting and filtering. 

  \sa QMailAccount, QSortFilterProxyModel
*/

/*!
  \enum QMailAccountListModel::Roles

  Represents common display roles of an account. These roles are used to display common account elements 
  in a view and its attached delegates.

  \value NameTextRole The name of the account 
  \value MessageTypeRole The type of the account 
  \value MessageSourcesRole The list of message sources for the account 
  \value MessageSinksRole The list of message sinks for the account 
*/

/*!
    Constructs a QMailAccountListModel with a parent \a parent.
    By default, the model will match all accounts in the database, and display them in
    the order they were submitted. Synchronization defaults to true.
*/

QMailAccountListModel::QMailAccountListModel(QObject* parent)
:
    QAbstractListModel(parent),
    d(new QMailAccountListModelPrivate(QMailAccountKey(),QMailAccountSortKey(),true))
{
    connect(QMailStore::instance(),
            SIGNAL(accountsAdded(QMailAccountIdList)),
            this,
            SLOT(accountsAdded(QMailAccountIdList)));
    connect(QMailStore::instance(),
            SIGNAL(accountsRemoved(QMailAccountIdList)),
            this,
            SLOT(accountsRemoved(QMailAccountIdList)));
    connect(QMailStore::instance(),
            SIGNAL(accountsUpdated(QMailAccountIdList)),
            this,
            SLOT(accountsUpdated(QMailAccountIdList)));
}

/*!
    Deletes the QMailMessageListModel object.
*/

QMailAccountListModel::~QMailAccountListModel()
{
    delete d; d = 0;
}

/*!
    \reimp
*/

int QMailAccountListModel::rowCount(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return d->ids().count();
}

/*!
    \reimp
*/

QVariant QMailAccountListModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return QVariant();

    int offset = index.row();
    QMailAccountId id = d->ids().at(offset);
    if (id == d->deletionId)
        return QVariant();

    QMailAccount account(id);

    switch(role)
    {
       case Qt::DisplayRole:
       case NameTextRole:
            return account.name();

       case MessageTypeRole:
            return static_cast<int>(account.messageType());

       case MessageSourcesRole:
            return account.messageSources();

       case MessageSinksRole:
            return account.messageSinks();
    }

    return QVariant();
}

/*!
    Returns the QMailAccountKey used to populate the contents of this model.
*/

QMailAccountKey QMailAccountListModel::key() const
{
    return d->key; 
}

/*!
    Sets the QMailAccountKey used to populate the contents of the model to \a key.
    If the key is empty, the model is populated with all the accounts from the 
    database.
*/

void QMailAccountListModel::setKey(const QMailAccountKey& key) 
{
    beginResetModel();
    d->key = key;
    d->init = false;
    endResetModel();
}

/*!
    Returns the QMailAccountSortKey used to sort the contents of the model.
*/

QMailAccountSortKey QMailAccountListModel::sortKey() const
{
   return d->sortKey;
}

/*!
    Sets the QMailAccountSortKey used to sort the contents of the model to \a sortKey.
    If the sort key is invalid, no sorting is applied to the model contents and accounts 
    are displayed in the order in which they were added into the database.
*/

void QMailAccountListModel::setSortKey(const QMailAccountSortKey& sortKey) 
{
    beginResetModel();
    d->sortKey = sortKey;
    d->init = false;
    endResetModel();
}

/*! \internal */

void QMailAccountListModel::accountsAdded(const QMailAccountIdList& ids)
{
    d->needSynchronize = true;
    if(!d->synchronizeEnabled)
        return;

    //TODO change this code to use our own searching and insertion routines
    //for more control
    //use id sorted indexes
    
    if(!d->init)
        d->initialize();
    
    QMailAccountKey passKey = d->key & QMailAccountKey::id(ids);
    QMailAccountIdList results = QMailStore::instance()->queryAccounts(passKey);
    if(results.isEmpty())
        return;

    if(results.count() > fullRefreshCutoff)
        fullRefresh();

    if(!d->sortKey.isEmpty())
    { 
        foreach(const QMailAccountId &id,results)
        {
            LessThanFunctorA lessThan(d->sortKey);

            //if sorting the list fails, then resort to a complete refresh
            if(lessThan.invalidatedList())
                fullRefresh();
            else
            {
                QMailAccountIdList::iterator itr = d->lowerBound(id, lessThan);
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
        foreach(const QMailAccountId &id,results)
            d->idList.append(id);
        endInsertRows();
    }
    d->needSynchronize = false;
}

/*! \internal */

void QMailAccountListModel::accountsUpdated(const QMailAccountIdList& ids)
{
    d->needSynchronize = true;
    if(!d->synchronizeEnabled)
        return;

    //TODO change this code to use our own searching and insertion routines
    //for more control
    //use id sorted indexes

    if(!d->init)
        d->initialize();

    QMailAccountKey idKey(QMailAccountKey::id(ids));

    QMailAccountIdList validIds = QMailStore::instance()->queryAccounts(idKey & d->key);
    if(validIds.count() > fullRefreshCutoff)
    {
        fullRefresh();
        return;
    }

    // Remove invalid ids from model
    foreach(const QMailAccountId &id,ids)
    {
        if (!validIds.contains(id))
        {
            //get the index
            int index = d->idList.indexOf(id);
            if (index == -1)
                continue;

            d->deletionId = id;
            beginRemoveRows(QModelIndex(),index,index);
            d->idList.removeAt(index);
            endRemoveRows();
            d->deletionId = QMailAccountId();
        }
    }

    LessThanFunctorA lessThan(d->sortKey);

    foreach(const QMailAccountId &id, validIds)
    {
        int index = d->idList.indexOf(id);
        if(index == -1) //insert
        {
            if(lessThan.invalidatedList())
                fullRefresh();
            else
            {
                QMailAccountIdList::iterator itr = d->lowerBound(id, lessThan);
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
                QMailAccountIdList::iterator itr = d->lowerBound(id, lessThan);
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
                    d->deletionId = QMailAccountId();

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

void QMailAccountListModel::accountsRemoved(const QMailAccountIdList& ids)
{
    d->needSynchronize = true;
    if(!d->synchronizeEnabled)
        return;

    if(!d->init)
        d->initialize();

    foreach(const QMailAccountId &id, ids)
    {
        int index = d->indexOf(id);
        if(index == -1)
            continue;

        d->deletionId = id;
        beginRemoveRows(QModelIndex(),index,index);
        d->idList.removeAt(index);
        endRemoveRows();
        d->deletionId = QMailAccountId();
    }
    d->needSynchronize = false;
}

/*!
    Returns the QMailAccountId of the account represented by the QModelIndex \a index.
    If the index is not valid an invalid QMailAccountId is returned.
*/

QMailAccountId QMailAccountListModel::idFromIndex(const QModelIndex& index) const
{
    if(!index.isValid())
        return QMailAccountId();

    return d->ids().at(index.row());
}

/*!
    Returns the QModelIndex that represents the account with QMailAccountId \a id.
    If the id is not conatained in this model, an invalid QModelIndex is returned.
*/

QModelIndex QMailAccountListModel::indexFromId(const QMailAccountId& id) const
{
    //if the id does not exist return null
    int index = d->indexOf(id);
    if(index != -1) {
        return createIndex(index,0);
    }

    return QModelIndex();
}

/*!
    Returns \c true if the model sychronizes its contents based on account changes
    in the database, otherwise returns \c false.
*/

bool QMailAccountListModel::synchronizeEnabled() const
{
    return d->synchronizeEnabled;
}

/*!
    Sets wheather the model synchronizes its contents based on account changes
    in the database to \a val.
*/

void QMailAccountListModel::setSynchronizeEnabled(bool val)
{
    d->synchronizeEnabled = val;
    if(val && d->needSynchronize)
        fullRefresh();
}

/*! \internal */

void QMailAccountListModel::fullRefresh()
{
    beginResetModel();
    d->init = false;
    endResetModel();
}

