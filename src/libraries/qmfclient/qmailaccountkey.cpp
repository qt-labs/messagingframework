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

#include "qmailaccountkey.h"
#include "qmailaccountkey_p.h"

#include <QStringList>
#include <QDateTime>

using namespace QMailDataComparator;

/*!
    \class QMailAccountKey

    \preliminary
    \brief The QMailAccountKey class defines the parameters used for querying a subset of
    all available accounts from the mail store.
    \ingroup messaginglibrary

    A QMailAccountKey is composed of an account property, an optional comparison operator
    and a comparison value. The QMailAccountKey class is used in conjunction with the
    QMailStore::queryAccounts() and QMailStore::countAccounts() functions to filter results
    which meet the criteria defined by the key.

    QMailAccountKey's can be combined using the logical operators (&), (|) and (~) to
    build more sophisticated queries.

    For example:

    To create a query for all accounts handling email messages:
    \code
    QMailAccountKey emailKey(QMailAccountKey::messageType(QMailMessage::Email));
    QMailAccountIdList results = QMailStore::instance()->queryAccounts(emailKey);
    \endcode

    To query all accounts handling SMS or MMS messages:
    \code
    QMailAccountKey mmsAccount(QMailAccountKey::messageType(QMailMessage::Mms));
    QMailAccountKey smsAccount(QMailAccountKey::messageType(QMailMessage::Sms));
    QMailAccountIdList results = QMailStore::instance()->queryAccounts(mmsAccount | smsAccount);
    \endcode

    \sa QMailStore, QMailAccount
*/

/*!
    \enum QMailAccountKey::Property

    This enum type describes the queryable data properties of a QMailAccount.

    \value Id The ID of the account.
    \value Name The name of the account.
    \value MessageType The type of messages handled by the account.
    \value FromAddress The address from which the account's outgoing messages should be reported as originating.
    \value Status The status value of the account.
    \value Custom The custom fields of the account.
    \value LastSynchronized The most recent time that a check for new mail in all folders of the account was completed successfully.
    \value IconPath The icon path of the account.
*/

/*!
    \typedef QMailAccountKey::IdType
    \internal
*/

/*!
    \typedef QMailAccountKey::ArgumentType

    Defines the type used to represent a single criterion of an account filter.

    Synonym for QMailKeyArgument<QMailAccountKey::Property>.
*/

Q_IMPLEMENT_USER_METATYPE(QMailAccountKey)

/*!
    Creates a QMailAccountKey without specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) matches all accounts.

    \sa isEmpty()
*/
QMailAccountKey::QMailAccountKey()
    : d(new QMailAccountKeyPrivate)
{
}

/*! 
    Constructs a QMailAccountKey which defines a query parameter where
    QMailAccount::Property \a p is compared using comparison operator
    \a c with a value \a value.
*/
QMailAccountKey::QMailAccountKey(Property p, const QVariant& value, QMailKey::Comparator c)
    : d(new QMailAccountKeyPrivate(p, value, c))
{
}

/*! 
    \fn QMailAccountKey::QMailAccountKey(const ListType &, Property, QMailKey::Comparator)
    \internal 
*/
template <typename ListType>
QMailAccountKey::QMailAccountKey(const ListType &valueList, QMailAccountKey::Property p, QMailKey::Comparator c)
    : d(new QMailAccountKeyPrivate(valueList, p, c))
{
}

/*!
    Creates a copy of the QMailAccountKey \a other.
*/
QMailAccountKey::QMailAccountKey(const QMailAccountKey& other)
{
    d = other.d;
}

/*!
    Destroys this QMailAccountKey.
*/
QMailAccountKey::~QMailAccountKey()
{
}

/*!
    Returns a key that is the logical NOT of the value of this key.

    If this key is empty, the result will be a non-matching key; if this key is
    non-matching, the result will be an empty key.

    \sa isEmpty(), isNonMatching()
*/
QMailAccountKey QMailAccountKey::operator~() const
{
    return QMailAccountKeyPrivate::negate(*this);
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/
QMailAccountKey QMailAccountKey::operator&(const QMailAccountKey& other) const
{
    return QMailAccountKeyPrivate::andCombine(*this, other);
}

/*!
    Returns a key that is the logical OR of this key and the value of key \a other.
*/
QMailAccountKey QMailAccountKey::operator|(const QMailAccountKey& other) const
{
    return QMailAccountKeyPrivate::orCombine(*this, other);
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/
const QMailAccountKey& QMailAccountKey::operator&=(const QMailAccountKey& other)
{
    return QMailAccountKeyPrivate::andAssign(*this, other);
}

/*!
    Performs a logical OR with this key and the key \a other and assigns the result
    to this key.
*/
const QMailAccountKey& QMailAccountKey::operator|=(const QMailAccountKey& other)
{
    return QMailAccountKeyPrivate::orAssign(*this, other);
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns
    \c false otherwise.
*/
bool QMailAccountKey::operator==(const QMailAccountKey& other) const
{
    return d->operator==(*other.d);
}

/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/
bool QMailAccountKey::operator!=(const QMailAccountKey& other) const
{
    return !d->operator==(*other.d);
}

/*!
    Assign the value of the QMailAccountKey \a other to this.
*/
const QMailAccountKey& QMailAccountKey::operator=(const QMailAccountKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false.

    An empty key matches all accounts.

    The result of combining an empty key with a non-empty key is the original non-empty key.
    This is true regardless of whether the combination is formed by an AND or an OR operation.

    The result of combining two empty keys is an empty key.

    \sa isNonMatching()
*/
bool QMailAccountKey::isEmpty() const
{
    return d->isEmpty();
}

/*!
    Returns true if the key is a non-matching key; otherwise returns false.

    A non-matching key does not match any accounts.

    The result of ANDing a non-matching key with a matching key is a non-matching key.
    The result of ORing a non-matching key with a matching key is the original matching key.

    The result of combining two non-matching keys is a non-matching key.

    \sa nonMatchingKey(), isEmpty()
*/
bool QMailAccountKey::isNonMatching() const
{
    return d->isNonMatching();
}

/*!
    Returns true if the key's criteria should be negated in application.
*/
bool QMailAccountKey::isNegated() const
{
    return d->negated;
}

/*!
    Returns the QVariant representation of this QMailAccountKey.
*/
QMailAccountKey::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

/*!
    Returns the list of arguments to this QMailAccountKey.
*/
const QList<QMailAccountKey::ArgumentType> &QMailAccountKey::arguments() const
{
    return d->arguments;
}

/*!
    Returns the list of sub keys held by this QMailAccountKey.
*/
const QList<QMailAccountKey> &QMailAccountKey::subKeys() const
{
    return d->subKeys;
}

/*!
    Returns the combiner used to combine arguments or sub keys of this QMailAccountKey.
*/
QMailKey::Combiner QMailAccountKey::combiner() const
{
    return d->combiner;
}

/*!
    \fn QMailAccountKey::serialize(Stream &stream) const

    Writes the contents of a QMailAccountKey to a \a stream.
*/
template <typename Stream> void QMailAccountKey::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*!
    \fn QMailAccountKey::deserialize(Stream &stream)

    Reads the contents of a QMailAccountKey from \a stream.
*/
template <typename Stream> void QMailAccountKey::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

/*!
    Returns a key that does not match any accounts (unlike an empty key).

    \sa isNonMatching(), isEmpty()
*/
QMailAccountKey QMailAccountKey::nonMatchingKey()
{
    return QMailAccountKeyPrivate::nonMatchingKey();
}

/*! 
    Returns a key matching accounts whose identifier matches \a id, according to \a cmp.

    \sa QMailAccount::id()
*/
QMailAccountKey QMailAccountKey::id(const QMailAccountId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailAccountKey(Id, id, QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose identifier is a member of \a ids, according to \a cmp.

    \sa QMailAccount::id()
*/
QMailAccountKey QMailAccountKey::id(const QMailAccountIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailAccountKey(ids, Id, QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailAccount::id()
*/
QMailAccountKey QMailAccountKey::id(const QMailAccountKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailAccountKey(Id, key, QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose name matches \a value, according to \a cmp.

    \sa QMailAccount::name()
*/
QMailAccountKey QMailAccountKey::name(const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailAccountKey(Name, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose name matches the substring \a value, according to \a cmp.

    \sa QMailAccount::name()
*/
QMailAccountKey QMailAccountKey::name(const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailAccountKey(Name, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose name is a member of \a values, according to \a cmp.

    \sa QMailAccount::name()
*/
QMailAccountKey QMailAccountKey::name(const QStringList &values, QMailDataComparator::InclusionComparator cmp)
{
    return QMailAccountKey(values, Name, QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose messageType matches \a value, according to \a cmp.

    \sa QMailAccount::messageType()
*/
QMailAccountKey QMailAccountKey::messageType(QMailMessageMetaDataFwd::MessageType value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailAccountKey(MessageType, static_cast<int>(value), QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose messageType is a bitwise match to \a value, according to \a cmp.

    \sa QMailAccount::messageType()
*/
QMailAccountKey QMailAccountKey::messageType(int value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailAccountKey(MessageType, value, QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose address matches \a value, according to \a cmp.

    \sa QMailAccount::fromAddress()
*/
QMailAccountKey QMailAccountKey::fromAddress(const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailAccountKey(FromAddress, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose address matches the substring \a value, according to \a cmp.

    \sa QMailAccount::fromAddress()
*/
QMailAccountKey QMailAccountKey::fromAddress(const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailAccountKey(FromAddress, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose last synchronization time matches the time \a value, according to \a cmp.

    \sa QMailAccount::lastSynchronized()
*/
QMailAccountKey QMailAccountKey::lastSynchronized(const QDateTime &value, QMailDataComparator::EqualityComparator cmp)
{
    // An invalid QDateTime does not exist-compare correctly, so use a substitute value
    return QMailAccountKey(LastSynchronized, (value.isNull() ? QDateTime::fromTime_t(0) : value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching accounts whose last synchronization timestamp has the relation to \a value that is specified by \a cmp.

    \sa QMailAccount::lastSynchronized()
*/
QMailAccountKey QMailAccountKey::lastSynchronized(const QDateTime &value, QMailDataComparator::RelationComparator cmp)
{
    return QMailAccountKey(LastSynchronized, value, QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose status matches \a value, according to \a cmp.

    \sa QMailAccount::status()
*/
QMailAccountKey QMailAccountKey::status(quint64 value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailAccountKey(Status, value, QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts whose status is a bitwise match to \a mask, according to \a cmp.

    \sa QMailAccount::status()
*/
QMailAccountKey QMailAccountKey::status(quint64 mask, QMailDataComparator::InclusionComparator cmp)
{
    return QMailAccountKey(Status, mask, QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts that possess a custom field with the name \a name, according to \a cmp.

    \sa QMailAccount::customField()
*/
QMailAccountKey QMailAccountKey::customField(const QString &name, QMailDataComparator::PresenceComparator cmp)
{
    return QMailAccountKey(Custom, QStringList() << QMailKey::stringValue(name), QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts that possess a custom field with the name \a name, whose value matches \a value, according to \a cmp.

    \sa QMailAccount::customField()
*/
QMailAccountKey QMailAccountKey::customField(const QString &name, const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailAccountKey(Custom, QStringList() << QMailKey::stringValue(name) << QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*! 
    Returns a key matching accounts that possess a custom field with the name \a name, whose value matches the substring \a value, according to \a cmp.

    \sa QMailAccount::customField()
*/
QMailAccountKey QMailAccountKey::customField(const QString &name, const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailAccountKey(Custom, QStringList() << QMailKey::stringValue(name) << QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching accounts whose icon path matches \a value, according to \a cmp.

    \sa QMailAccount::iconPath()
*/
QMailAccountKey QMailAccountKey::iconPath(const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailAccountKey(IconPath, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching accounts whose icon path matches the substring \a value, according to \a cmp.

    \sa QMailAccount::iconPath()()
*/
QMailAccountKey QMailAccountKey::iconPath(const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailAccountKey(IconPath, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching accounts whose icon path is a member of \a values, according to \a cmp.

    \sa QMailAccount::iconPath()
*/
QMailAccountKey QMailAccountKey::iconPath(const QStringList &values, QMailDataComparator::InclusionComparator cmp)
{
    return QMailAccountKey(values, IconPath, QMailKey::comparator(cmp));
}
