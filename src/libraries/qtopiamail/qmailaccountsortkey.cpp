/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailaccountsortkey.h"
#include "qmailaccountsortkey_p.h"

/*!
    \class QMailAccountSortKey

    \preliminary
    \brief The QMailAccountSortKey class defines the parameters used for sorting a subset of 
    queried accounts from the store.
    \ingroup messaginglibrary

    A QMailAccountSortKey is composed of an account property to sort and a sort order. 
    The QMailAccountSortKey class is used in conjunction with the QMailStore::query() 
    function to sort account results according to the criteria defined by the sort key.

    For example:
    To create a query for all accounts sorted by the name in ascending order:
    \code
    QMailAccountSortKey sortNameKey(QMailAccountSortKey::name(Qt::Ascending));
    QMailAccountIdList results = QMailStore::instance()->query(sortNameKey);
    \endcode
    
    \sa QMailStore
*/

/*!
    \enum QMailAccountSortKey::Property

    This enum type describes the sortable data properties of a QMailFolder.

    \value Id The ID of the account.
    \value Name The name of the account.
    \value MessageType The type of messages handled by the account.
    \value Status The status value of the account.
*/

/*!
    \typedef QMailAccountSortKey::ArgumentType
    
    Defines the type used to represent a single sort criterion of an account sort key.

    Synonym for QPair<QMailAccountKey::Property, Qt::SortOrder>.
*/

/*!
    Create a QMailAccountSortKey with specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) sorts no folders. 

    The result of combining an empty key with a non-empty key is the same as the original 
    non-empty key.

    The result of combining two empty keys is an empty key.
*/

QMailAccountSortKey::QMailAccountSortKey()
    : d(new QMailAccountSortKeyPrivate())
{
}

/*!
    Construct a QMailAccountSortKey which sorts a set of results based on the  
    QMailAccountSortKey::Property \a p and the Qt::SortOrder \a order 
*/

QMailAccountSortKey::QMailAccountSortKey(Property p, Qt::SortOrder order)
    : d(new QMailAccountSortKeyPrivate())
{
    d->arguments.append(ArgumentType(p, order));
}

/*!
    Create a copy of the QMailAccountSortKey \a other.
*/

QMailAccountSortKey::QMailAccountSortKey(const QMailAccountSortKey& other)
    : d(new QMailAccountSortKeyPrivate())
{
    this->operator=(other);
}

/*!
    Destroys this QMailAccountSortKey.
*/

QMailAccountSortKey::~QMailAccountSortKey()
{
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/

QMailAccountSortKey QMailAccountSortKey::operator&(const QMailAccountSortKey& other) const
{
    QMailAccountSortKey k;
    k.d->arguments = d->arguments + other.d->arguments;
    return k;
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/

QMailAccountSortKey& QMailAccountSortKey::operator&=(const QMailAccountSortKey& other)
{
    *this = *this & other;
    return *this;
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns 
    \c false otherwise.
*/

bool QMailAccountSortKey::operator==(const QMailAccountSortKey& other) const
{
    return d->arguments == other.d->arguments;
}
/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/

bool QMailAccountSortKey::operator!=(const QMailAccountSortKey& other) const
{
   return !(*this == other); 
}

/*!
    Assign the value of the QMailAccountSortKey \a other to this.
*/

QMailAccountSortKey& QMailAccountSortKey::operator=(const QMailAccountSortKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false.
*/

bool QMailAccountSortKey::isEmpty() const
{
    return d->arguments.isEmpty();
}

/*!
  Returns the list of arguments to this QMailAccountSortKey.
*/

const QList<QMailAccountSortKey::ArgumentType> &QMailAccountSortKey::arguments() const
{
    return d->arguments;
}

/*!
    Returns a key that sorts accounts by their identifiers, according to \a order.

    \sa QMailAccount::id()
*/
QMailAccountSortKey QMailAccountSortKey::id(Qt::SortOrder order)
{
    return QMailAccountSortKey(Id, order);
}

/*!
    Returns a key that sorts accounts by their names, according to \a order.

    \sa QMailAccount::name()
*/
QMailAccountSortKey QMailAccountSortKey::name(Qt::SortOrder order)
{
    return QMailAccountSortKey(Name, order);
}

/*!
    Returns a key that sorts accounts by the message type they handle, according to \a order.

    \sa QMailAccount::messageType()
*/
QMailAccountSortKey QMailAccountSortKey::messageType(Qt::SortOrder order)
{
    return QMailAccountSortKey(MessageType, order);
}

/*!
    Returns a key that sorts accounts by their status values, according to \a order.

    \sa QMailAccount::status()
*/
QMailAccountSortKey QMailAccountSortKey::status(Qt::SortOrder order)
{
    return QMailAccountSortKey(Status, order);
}

