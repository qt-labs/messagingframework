/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailfoldersortkey.h"
#include "qmailfoldersortkey_p.h"


/*!
    \class QMailFolderSortKey
    \inpublicgroup QtMessagingModule

    \preliminary
    \brief The QMailFolderSortKey class defines the parameters used for sorting a subset of 
    queried folders from the mail store.
    \ingroup messaginglibrary

    A QMailFolderSortKey is composed of a folder property to sort and a sort order. 
    The QMailFolderSortKey class is used in conjunction with the QMailStore::queryFolders() 
    function to sort folder results according to the criteria defined by the sort key.

    For example:
    To create a query for all folders sorted by the path in ascending order:
    \code
    QMailFolderSortKey sortKey(QMailFolderSortKey::path(Qt::Ascending));
    QMailIdList results = QMailStore::instance()->queryFolders(QMailFolderKey(), sortKey);
    \endcode
    
    \sa QMailStore, QMailFolderKey
*/

/*!
    \enum QMailFolderSortKey::Property

    This enum type describes the sortable data properties of a QMailFolder.

    \value Id The ID of the folder.
    \value Path The path of the folder in native form.
    \value ParentFolderId The ID of the parent folder for a given folder.
    \value ParentAccountId The ID of the parent account for a given folder.
    \value DisplayName The name of the folder, designed for display to users.
    \value Status The status value of the folder.
    \value ServerCount The number of messages reported to be on the server for the folder.
    \value ServerUnreadCount The number of unread messages reported to be on the server for the folder.
    \value ServerUndiscoveredCount The number of undiscovered messages reported to be on the server for the folder.
*/

/*!
    \typedef QMailFolderSortKey::ArgumentType
    
    Defines the type used to represent a single sort criterion of a folder sort key.

    Synonym for QPair<QMailFolderKey::Property, Qt::SortOrder>.
*/

/*!
    Create a QMailFolderSortKey with specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) sorts no folders. 

    The result of combining an empty key with a non-empty key is the same as the original 
    non-empty key.

    The result of combining two empty keys is an empty key.
*/

QMailFolderSortKey::QMailFolderSortKey()
    : d(new QMailFolderSortKeyPrivate())
{
}

/*!
    Construct a QMailFolderSortKey which sorts a set of results based on the  
    QMailFolderSortKey::Property \a p and the Qt::SortOrder \a order 
*/

QMailFolderSortKey::QMailFolderSortKey(Property p, Qt::SortOrder order)
    : d(new QMailFolderSortKeyPrivate())
{
    d->arguments.append(ArgumentType(p, order));
}

/*!
    Create a copy of the QMailFolderSortKey \a other.
*/

QMailFolderSortKey::QMailFolderSortKey(const QMailFolderSortKey& other)
    : d(new QMailFolderSortKeyPrivate())
{
    this->operator=(other);
}

/*!
    Destroys this QMailFolderSortKey.
*/

QMailFolderSortKey::~QMailFolderSortKey()
{
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/

QMailFolderSortKey QMailFolderSortKey::operator&(const QMailFolderSortKey& other) const
{
    QMailFolderSortKey k;
    k.d->arguments = d->arguments + other.d->arguments;
    return k;
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/

QMailFolderSortKey& QMailFolderSortKey::operator&=(const QMailFolderSortKey& other)
{
    *this = *this & other;
    return *this;
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns 
    \c false otherwise.
*/

bool QMailFolderSortKey::operator==(const QMailFolderSortKey& other) const
{
    return d->arguments == other.d->arguments;
}
/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/

bool QMailFolderSortKey::operator!=(const QMailFolderSortKey& other) const
{
   return !(*this == other); 
}

/*!
    Assign the value of the QMailFolderSortKey \a other to this.
*/

QMailFolderSortKey& QMailFolderSortKey::operator=(const QMailFolderSortKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false.
*/

bool QMailFolderSortKey::isEmpty() const
{
    return d->arguments.isEmpty();
}

/*!
  Returns the list of arguments to this QMailFolderSortKey.
*/

const QList<QMailFolderSortKey::ArgumentType> &QMailFolderSortKey::arguments() const
{
    return d->arguments;
}

/*!
    Returns a key that sorts folders by their identifiers, according to \a order.

    \sa QMailFolder::id()
*/
QMailFolderSortKey QMailFolderSortKey::id(Qt::SortOrder order)
{
    return QMailFolderSortKey(Id, order);
}

/*!
    Returns a key that sorts folders by their paths, according to \a order.

    \sa QMailFolder::path()
*/
QMailFolderSortKey QMailFolderSortKey::path(Qt::SortOrder order)
{
    return QMailFolderSortKey(Path, order);
}

/*!
    Returns a key that sorts folders by their parent folders' identifiers, according to \a order.

    \sa QMailFolder::parentFolderId()
*/
QMailFolderSortKey QMailFolderSortKey::parentFolderId(Qt::SortOrder order)
{
    return QMailFolderSortKey(ParentFolderId, order);
}

/*!
    Returns a key that sorts folders by their parent accounts' identifiers, according to \a order.

    \sa QMailFolder::parentAccountId()
*/
QMailFolderSortKey QMailFolderSortKey::parentAccountId(Qt::SortOrder order)
{
    return QMailFolderSortKey(ParentAccountId, order);
}

/*!
    Returns a key that sorts folders by their display name, according to \a order.

    \sa QMailFolder::displayName()
*/
QMailFolderSortKey QMailFolderSortKey::displayName(Qt::SortOrder order)
{
    return QMailFolderSortKey(DisplayName, order);
}

/*!
    Returns a key that sorts folders by their status values, according to \a order.

    \sa QMailFolder::status()
*/
QMailFolderSortKey QMailFolderSortKey::status(Qt::SortOrder order)
{
    return QMailFolderSortKey(Status, order);
}

/*!
    Returns a key that sorts folders by their message count on server, according to \a order.

    \sa QMailFolder::status()
*/
QMailFolderSortKey QMailFolderSortKey::serverCount(Qt::SortOrder order)
{
    return QMailFolderSortKey(ServerCount, order);
}

/*!
    Returns a key that sorts folders by their message unread count on server, according to \a order.

    \sa QMailFolder::status()
*/
QMailFolderSortKey QMailFolderSortKey::serverUnreadCount(Qt::SortOrder order)
{
    return QMailFolderSortKey(ServerUnreadCount, order);
}

/*!
    Returns a key that sorts folders by their message undiscovered count on server, according to \a order.

    \sa QMailFolder::status()
*/
QMailFolderSortKey QMailFolderSortKey::serverUndiscoveredCount(Qt::SortOrder order)
{
    return QMailFolderSortKey(ServerUndiscoveredCount, order);
}

