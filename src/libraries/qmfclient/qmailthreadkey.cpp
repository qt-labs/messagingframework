/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmailthreadkey.h"
#include "qmailthreadkey_p.h"

#include "qmailaccountkey.h"
#include <QStringList>

using namespace QMailKey;

/*!
    \class QMailFolderKey

    \preliminary
    \brief The QMailFolderKey class defines the parameters used for querying a subset of
    all available folders from the mail store.
    \ingroup messaginglibrary

    A QMailFolderKey is composed of a folder property, an optional comparison operator
    and a comparison value. The QMailFolderKey class is used in conjunction with the 
    QMailStore::queryFolders() and QMailStore::countFolders() functions to filter results 
    which meet the criteria defined by the key.

    QMailFolderKey's can be combined using the logical operators (&), (|) and (~) to
    build more sophisticated queries.

    For example:

    To create a query for folders with paths containing "inbox" or "sms":
    \code
    QMailFolderKey inboxKey(QMailFolderKey::path("inbox", QMailDataComparator::Includes));
    QMailFolderKey smsKey(QMailFolderKey::path("sms", QMailDataComparator::Includes));
    QMailFolderIdList results = QMailStore::instance()->queryFolders(inboxKey | smsKey);
    \endcode

    To query all folders with name containing "foo" for a specified account:
    \code
    QMailFolderIdList fooFolders(const QMailAccountId& accountId)
    {
        QMailFolderKey nameKey(QMailFolderKey::displayName("foo", QMailDataComparator::Includes);
        QMailFolderKey accountKey(QMailFolderKey::parentAccountId(accountId));

        return QMailStore::instance()->queryFolders(nameKey & accountKey);
    }
    \endcode

    \sa QMailStore, QMailFolder
*/

/*!
    \enum QMailFolderKey::Property

    This enum type describes the queryable data properties of a QMailFolder.

    \value Id The ID of the folder.
    \value Path The path of the folder in native form.
    \value ParentFolderId The ID of the parent folder for a given folder.
    \value ParentAccountId The ID of the parent account for this folder.
    \value DisplayName The name of the folder, designed for display to users.
    \value Status The status value of the folder.
    \value AncestorFolderIds The set of IDs of folders which are direct or indirect parents of this folder.
    \value ServerCount The number of messages reported to be on the server for the folder.
    \value ServerUnreadCount The number of unread messages reported to be on the server for the folder.
    \value ServerUndiscoveredCount The number of undiscovered messages reported to be on the server for the folder.
    \value Custom The custom fields of the folder.
*/

/*!
    \typedef QMailFolderKey::IdType
    \internal
*/

/*!
    \typedef QMailFolderKey::ArgumentType
    
    Defines the type used to represent a single criterion of a folder filter.

    Synonym for QMailKeyArgument<QMailFolderKey::Property>.
*/

Q_IMPLEMENT_USER_METATYPE(QMailThreadKey);

/*!
    Creates a QMailFolderKey without specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) matches all folders. 

    \sa isEmpty()
*/
QMailThreadKey::QMailThreadKey()
    : d(new QMailThreadKeyPrivate)
{
}

/*!


*/
QMailThreadKey::QMailThreadKey(Property p, const QVariant& value, QMailKey::Comparator c)
    : d(new QMailThreadKeyPrivate(p, value, c))
{
}

/*! 
    \fn QMailThreadKey::QMailThreadKey(const ListType &, Property, QMailKey::Comparator)
    \internal 
*/
template <typename ListType>
QMailThreadKey::QMailThreadKey(const ListType &valueList, QMailThreadKey::Property p, QMailKey::Comparator c)
    : d(new QMailThreadKeyPrivate(valueList, p, c))
{
}

/*!
    Creates a copy of the QMailThreadKey \a other.
*/
QMailThreadKey::QMailThreadKey(const QMailThreadKey& other)
{
    d = other.d;
}

/*!
    Destroys this QMailFolderKey.
*/
QMailThreadKey::~QMailThreadKey()
{
}

/*!
    Returns a key that is the logical NOT of the value of this key.

    If this key is empty, the result will be a non-matching key; if this key is 
    non-matching, the result will be an empty key.

    \sa isEmpty(), isNonMatching()
*/
QMailThreadKey QMailThreadKey::operator~() const
{
    return QMailThreadKeyPrivate::negate(*this);
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/
QMailThreadKey QMailThreadKey::operator&(const QMailThreadKey& other) const
{
    return QMailThreadKeyPrivate::andCombine(*this, other);
}

/*!
    Returns a key that is the logical OR of this key and the value of key \a other.
*/
QMailThreadKey QMailThreadKey::operator|(const QMailThreadKey& other) const
{
    return QMailThreadKeyPrivate::orCombine(*this, other);
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/
const QMailThreadKey& QMailThreadKey::operator&=(const QMailThreadKey& other)
{
    return QMailThreadKeyPrivate::andAssign(*this, other);
}

/*!
    Performs a logical OR with this key and the key \a other and assigns the result
    to this key.
*/
const QMailThreadKey& QMailThreadKey::operator|=(const QMailThreadKey& other)
{
    return QMailThreadKeyPrivate::orAssign(*this, other);
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns 
    \c false otherwise.
*/
bool QMailThreadKey::operator==(const QMailThreadKey& other) const
{
    return d->operator==(*other.d);
}

/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/
bool QMailThreadKey::operator!=(const QMailThreadKey& other) const
{
    return !d->operator==(*other.d);
}

/*!
    Assign the value of the QMailFolderKey \a other to this.
*/
const QMailThreadKey& QMailThreadKey::operator=(const QMailThreadKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false. 

    An empty key matches all folders.

    The result of combining an empty key with a non-empty key is the original non-empty key. 
    This is true regardless of whether the combination is formed by an AND or an OR operation.

    The result of combining two empty keys is an empty key.

    \sa isNonMatching()
*/
bool QMailThreadKey::isEmpty() const
{
    return d->isEmpty();
}

/*!
    Returns true if the key is a non-matching key; otherwise returns false.

    A non-matching key does not match any folders.

    The result of ANDing a non-matching key with a matching key is a non-matching key.
    The result of ORing a non-matching key with a matching key is the original matching key.

    The result of combining two non-matching keys is a non-matching key.

    \sa nonMatchingKey(), isEmpty()
*/
bool QMailThreadKey::isNonMatching() const
{
    return d->isNonMatching();
}

/*! 
    Returns true if the key's criteria should be negated in application.
*/
bool QMailThreadKey::isNegated() const
{
    return d->negated;
}

/*!
    Returns the QVariant representation of this QMailFolderKey. 
*/
QMailThreadKey::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

/*!
    Returns the list of arguments to this QMailFolderKey.
*/
const QList<QMailThreadKey::ArgumentType> &QMailThreadKey::arguments() const
{
    return d->arguments;
}

/*!
    Returns the list of sub keys held by this QMailFolderKey.
*/
const QList<QMailThreadKey> &QMailThreadKey::subKeys() const
{
    return d->subKeys;
}

/*! 
    Returns the combiner used to combine arguments or sub keys of this QMailFolderKey.
*/
QMailKey::Combiner QMailThreadKey::combiner() const
{
    return d->combiner;
}

/*!
    \fn QMailThreadKey::serialize(Stream &stream) const

    Writes the contents of a QMailFolderKey to a \a stream.
*/
template <typename Stream> void QMailThreadKey::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*!
    \fn QMailFolderKey::deserialize(Stream &stream)

    Reads the contents of a QMailFolderKey from \a stream.
*/
template <typename Stream> void QMailThreadKey::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

/*!
    Returns a key that does not match any folders (unlike an empty key).

    \sa isNonMatching(), isEmpty()
*/
QMailThreadKey QMailThreadKey::nonMatchingKey()
{
    return QMailThreadKeyPrivate::nonMatchingKey();
}

/*!
    Returns a key matching folders whose identifier matches \a id, according to \a cmp.

    \sa QMailFolder::id()
*/
QMailThreadKey QMailThreadKey::id(const QMailThreadId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailThreadKey(Id, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching threads whose identifier is a member of \a ids, according to \a cmp.

    \sa QMailFolder::id()
*/
QMailThreadKey QMailThreadKey::id(const QMailThreadIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(ids, Id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailFolder::id()
*/
QMailThreadKey QMailThreadKey::id(const QMailThreadKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(Id, key, QMailKey::comparator(cmp));
}


/*!
	Returns a key matching messages whose serverUid matches \a uid, according to \a cmp.

	\sa QMailMessage::serverUid()
*/
QMailThreadKey QMailThreadKey::serverUid(const QString &uid, QMailDataComparator::EqualityComparator cmp)
{
	return QMailThreadKey(ServerUid, QMailKey::stringValue(uid), QMailKey::comparator(cmp));
}

/*!
	Returns a key matching messages whose serverUid matches the substring \a uid, according to \a cmp.

	\sa QMailMessage::serverUid()
*/
QMailThreadKey QMailThreadKey::serverUid(const QString &uid, QMailDataComparator::InclusionComparator cmp)
{
	return QMailThreadKey(ServerUid, QMailKey::stringValue(uid), QMailKey::comparator(cmp));
}

/*!
	Returns a key matching messages whose serverUid is a member of \a uids, according to \a cmp.

	\sa QMailMessage::serverUid()
*/
QMailThreadKey QMailThreadKey::serverUid(const QStringList &uids, QMailDataComparator::InclusionComparator cmp)
{
	return QMailThreadKey(uids, ServerUid, QMailKey::comparator(cmp));
}

