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

#include "qmailthreadkey.h"
#include "qmailthreadkey_p.h"

#include "qmailaccountkey.h"
#include "qmailmessagekey.h"
#include <QDateTime>
#include <QStringList>

using namespace QMailKey;


Q_IMPLEMENT_USER_METATYPE(QMailThreadKey)


/*!
    \class QMailThreadKey

    \preliminary
    \brief The QMailThreadKey class defines the parameters used for querying a subset of
    all message threads from the mail store.
    \ingroup messaginglibrary

    A QMailThreadKey is composed of a thread property, an optional comparison operator
    and a comparison value. The QMailThreadKey class is used in conjunction with the
    QMailStore::queryThreads() and QMailStore::countThreads() functions to filter results
    which meet the criteria defined by the key.

    QMailThreadKeys can be combined using the logical operators (&), (|) and (~) to
    create more refined queries.

    \sa QMailStore, QMailThread
*/

/*!
    \enum QMailThreadKey::Property

    This enum type describes the data query properties of a QMailThread.

    \value Id The ID of the thread.
    \value ServerUid The ServerUid of thread.
    \value MessageCount The number of messages in the thread
    \value UnreadCount The number of unread messages in the thread.
    \value Includes List of message identifiers.
    \value ParentAccountId The identifier of the parent account of the thread.
    \value Custom For internal use, may be removed.
*/

/*!
    \typedef QMailThreadKey::IdType
    \internal
*/

/*!
    \typedef QMailThreadKey::ArgumentType

    Defines the type used to represent a single criterion of a message filter.

    Synonym for QMailThreadKeyArgument<QMailThreadKey::Property>.
*/

/*!
    Creates a QMailThreadKey without specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) matches all threads.

    \sa isEmpty()
*/
QMailThreadKey::QMailThreadKey()
    : d(new QMailThreadKeyPrivate)
{
}

/*!
    Constructs a QMailThreadKey which defines a query parameter where
    Property \a p is compared using comparison operator
    \a c with a value \a value.
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
    Destroys this QMailThreadKey.
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
    Assign the value of the QMailThreadKey \a other to this.
*/
const QMailThreadKey& QMailThreadKey::operator=(const QMailThreadKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false.

    An empty key matches all threads.

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

    A non-matching key does not match any threads.

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
    Returns the list of arguments to this QMailThreadKey.
*/
const QList<QMailThreadKey::ArgumentType> &QMailThreadKey::arguments() const
{
    return d->arguments;
}

/*!
    Returns the list of sub keys held by this QMailThreadKey.
*/
const QList<QMailThreadKey> &QMailThreadKey::subKeys() const
{
    return d->subKeys;
}

/*!
    Returns the combiner used to combine arguments or sub keys of this QMailThreadKey.
*/
QMailKey::Combiner QMailThreadKey::combiner() const
{
    return d->combiner;
}

/*!
    \fn QMailThreadKey::serialize(Stream &stream) const

    Writes the contents of a QMailThreadKey to a \a stream.
*/
template <typename Stream> void QMailThreadKey::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*!
    \fn QMailThreadKey::deserialize(Stream &stream)

    Reads the contents of a QMailThreadKey from \a stream.
*/
template <typename Stream> void QMailThreadKey::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

/*!
    Returns a key that does not match any threads (unlike an empty key).

    \sa isNonMatching(), isEmpty()
*/
QMailThreadKey QMailThreadKey::nonMatchingKey()
{
    return QMailThreadKeyPrivate::nonMatchingKey();
}

/*!
    Returns a key matching threads whose identifier matches \a id, according to \a cmp.

    \sa QMailFolder::id()
*/
QMailThreadKey QMailThreadKey::id(const QMailThreadId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailThreadKey(Id, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching threads whose identifier is a member of \a ids, according to \a cmp.

    \sa QMailThread::id()
*/
QMailThreadKey QMailThreadKey::id(const QMailThreadIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(ids, Id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching thread whose identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailThread::id()
*/
QMailThreadKey QMailThreadKey::id(const QMailThreadKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(Id, key, QMailKey::comparator(cmp));
}


/*!
    Returns a key matching threads whose serverUid matches \a uid, according to \a cmp.

    \sa QMailThread::serverUid()
*/
QMailThreadKey QMailThreadKey::serverUid(const QString &uid, QMailDataComparator::EqualityComparator cmp)
{
    return QMailThreadKey(ServerUid, QMailKey::stringValue(uid), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching threads whose serverUid matches the substring \a uid, according to \a cmp.

    \sa QMailThread::serverUid()
*/
QMailThreadKey QMailThreadKey::serverUid(const QString &uid, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(ServerUid, QMailKey::stringValue(uid), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching threads whose serverUid is a member of \a uids, according to \a cmp.

    \sa QMailThread::serverUid()
*/
QMailThreadKey QMailThreadKey::serverUid(const QStringList &uids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(uids, ServerUid, QMailKey::comparator(cmp));
}


/*!
    Returns a key matching threads that include a message in \a ids, according to \a cmp.
*/

QMailThreadKey QMailThreadKey::includes(const QMailMessageIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(ids, QMailThreadKey::Includes, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching threads that include a message in \a key, according to \a cmp.
*/

QMailThreadKey QMailThreadKey::includes(const QMailMessageKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(Includes, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching threads whose parent account's identifier is \a id  according to \a cmp.

    \sa QMailThread::parentAccountId()
*/
QMailThreadKey QMailThreadKey::parentAccountId(const QMailAccountId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailThreadKey(ParentAccountId, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching threads whose parent account's identifier is a member of \a ids, according to \a cmp.

    \sa QMailThread::parentAccountId()
*/
QMailThreadKey QMailThreadKey::parentAccountId(const QMailAccountIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(ids, ParentAccountId, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching threads whose parent account's identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailThread::parentAccountId()
*/
QMailThreadKey QMailThreadKey::parentAccountId(const QMailAccountKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(ParentAccountId, key, QMailKey::comparator(cmp));
}

QMailThreadKey QMailThreadKey::countMessages(const int count, QMailDataComparator::InclusionComparator cmp)
{
    return QMailThreadKey(MessageCount, count, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose LastDate matches \a value, according to \a cmp.

    \sa QMailThread::lastDate()
*/
QMailThreadKey QMailThreadKey::lastDate(const QDateTime &value, QMailDataComparator::EqualityComparator cmp)
{
    // An invalid QDateTime does not exist-compare correctly, so use a substitute value
    QDateTime x(value.isNull() ? QDateTime::fromTime_t(0) : value);
    return QMailThreadKey(LastDate, x, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose LastDate has the relation to \a value that is specified by \a cmp.

    \sa QMailThread::lastDate()
*/
QMailThreadKey QMailThreadKey::lastDate(const QDateTime &value, QMailDataComparator::RelationComparator cmp)
{
    return QMailThreadKey(LastDate, value, QMailKey::comparator(cmp));
}
